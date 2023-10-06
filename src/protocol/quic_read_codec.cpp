/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "quic_read_codec.hpp"

#include "../folly/io/Cursor.h"
#include "parse.hpp"
#include "quic_packet_num.hpp"
#include "quic_packet.hpp"
#include "quic_header.hpp"
#include "quic_ack.hpp"

namespace {
    quic::ConnectionId zeroConnId() {
        std::vector<uint8_t> zeroData(quic::kDefaultConnectionIdSize, 0);
        return quic::ConnectionId(zeroData);
    }
} // namespace

namespace quic {

QuicReadCodec::QuicReadCodec(QuicNodeType nodeType) : _nodeType(nodeType) {}

folly::Optional<VersionNegotiationPacket>
QuicReadCodec::tryParsingVersionNegotiation(BufQueue& queue) {
    folly::io::Cursor cursor(queue.front());
    if (!cursor.canAdvance(sizeof(uint8_t))) {
        return folly::none;
    }
    uint8_t initialByte = cursor.readBE<uint8_t>();
    auto headerForm = getHeaderForm(initialByte);
    if (headerForm != HeaderForm::Long) {
        return folly::none;
    }
    auto longHeaderInvariant = parseLongHeaderInvariant(initialByte, cursor);
    if (!longHeaderInvariant) {
        // if it is an invalid packet, it's definitely not a VN packet, so ignore
        // it.
        return folly::none;
    }
    if (longHeaderInvariant->invariant.version != QuicVersion::VERSION_NEGOTIATION) {
        return folly::none;
    }
    return decodeVersionNegotiation(*longHeaderInvariant, cursor);
}

folly::Expected<ParsedLongHeader, TransportErrorCode> tryParseLongHeader(folly::io::Cursor& cursor, QuicNodeType nodeType) {
    if (cursor.isAtEnd() || !cursor.canAdvance(sizeof(uint8_t))) {
        return folly::makeUnexpected(TransportErrorCode::PROTOCOL_VIOLATION);
    }
    auto initialByte = cursor.readBE<uint8_t>();
    auto longHeaderInvariant = parseLongHeaderInvariant(initialByte, cursor);
    if (!longHeaderInvariant) {
        // We've failed to parse the long header, so we have no idea where this
        // packet ends. Clear the queue since no other data in this packet is
        // parse-able.
        return folly::makeUnexpected(longHeaderInvariant.error());
    }
    if (longHeaderInvariant->invariant.version == QuicVersion::VERSION_NEGOTIATION) {
        // We shouldn't handle VN packets while parsing the long header.
        // We assume here that they have been handled before calling this
        // function.
        // Since VN is not allowed to be coalesced with another packet
        // type, we clear out the buffer to avoid anyone else parsing it.
        return folly::makeUnexpected(TransportErrorCode::PROTOCOL_VIOLATION);
    }
    auto type = parseLongHeaderType(initialByte);

    auto parsedLongHeader = parseLongHeaderVariants(type, *longHeaderInvariant, cursor, nodeType);
    if (!parsedLongHeader) {
        // We've failed to parse the long header, so we have no idea where this
        // packet ends. Clear the queue since no other data in this packet is
        // parse-able.
        return folly::makeUnexpected(parsedLongHeader.error());
    }

    return std::move(parsedLongHeader.value());
}

CodecResult QuicReadCodec::parseLongHeaderPacket(BufQueue& queue, const AckStates& ackStates) {
    folly::io::Cursor cursor(queue.front());
    const uint8_t initialByte = *cursor.peekBytes().data();

    auto res = tryParseLongHeader(cursor, _nodeType);
    if (res.hasError()) {
        queue.move();
        return CodecResult(Nothing());
    }
    auto parsedLongHeader = std::move(res.value());
    auto type = parsedLongHeader.header.getHeaderType();

    // As soon as we have parsed out the long header we can split off any
    // coalesced packets. We do this early since the spec mandates that decryption
    // failure must not stop the processing of subsequent coalesced packets.
    auto longHeader = std::move(parsedLongHeader.header);

    if (type == LongHeader::Types::Retry) {
        Buf integrityTag;
        cursor.clone(integrityTag, kRetryIntegrityTagLen);
        queue.move();
        return RetryPacket(std::move(longHeader), std::move(integrityTag), initialByte);
    }

    uint64_t packetNumberOffset = cursor.getCurrentPosition();
    size_t currentPacketLen = packetNumberOffset + parsedLongHeader.packetLength.packetLength;
    if (queue.chainLength() < currentPacketLen) {
        // Packet appears truncated, there's no parse-able data left.
        queue.move();
        return CodecResult(Nothing());
    }
    auto currentPacketData = queue.splitAtMost(currentPacketLen);
    cursor.reset(currentPacketData.get());
    cursor.skip(packetNumberOffset);
    // Sample starts after the max packet number size. This ensures that we
    // have enough bytes to skip before we can start reading the sample.
    if (!cursor.canAdvance(kMaxPacketNumEncodingSize)) {
    // Packet appears truncated, there's no parse-able data left.
        queue.move();
        return CodecResult(Nothing());
    }
    cursor.skip(kMaxPacketNumEncodingSize);
    Sample sample;
    if (!cursor.canAdvance(sample.size())) {
        // Packet appears truncated, there's no parse-able data left.
        queue.move();
        return CodecResult(Nothing());
    }
    cursor.pull(sample.data(), sample.size());
    const PacketNumberCipher* headerCipher{nullptr};
    const Aead* cipher{nullptr};
    auto protectionType = longHeader.getProtectionType();
    switch (protectionType) {
        case ProtectionType::Initial:
            if (!_initialHeaderCipher) {
                return CodecResult(Nothing());
            }
            headerCipher = _initialHeaderCipher.get();
            cipher = _initialReadCipher.get();
            break;
        case ProtectionType::Handshake:
            headerCipher = _handshakeHeaderCipher.get();
            cipher = _handshakeReadCipher.get();
            break;
        case ProtectionType::ZeroRtt:
            if (_handshakeDoneTime) {
                // TODO actually drop the 0-rtt keys in addition to dropping packets.
                auto timeBetween = Clock::now() - *_handshakeDoneTime;
                if (timeBetween > kTimeToRetainZeroRttKeys) {
                    return CodecResult(Nothing());
                }
            }
            headerCipher = _zeroRttHeaderCipher.get();
            cipher = _zeroRttReadCipher.get();
            break;
        case ProtectionType::KeyPhaseZero:
        case ProtectionType::KeyPhaseOne:
            //CHECK(false) << "one rtt protection type in long header";
    }
    if (!headerCipher || !cipher) {
        return CodecResult(CipherUnavailable(std::move(currentPacketData), protectionType));
    }

    PacketNum expectedNextPacketNum = 0;
    folly::Optional<PacketNum> largestRecvdPacketNum;
    switch (longHeaderTypeToProtectionType(type)) {
        case ProtectionType::Initial:
            largestRecvdPacketNum = ackStates.initialAckState->largestRecvdPacketNum;
            break;
        case ProtectionType::Handshake:
            largestRecvdPacketNum = ackStates.handshakeAckState->largestRecvdPacketNum;
            break;
        case ProtectionType::ZeroRtt:
            largestRecvdPacketNum = ackStates.appDataAckState.largestRecvdPacketNum;
            break;
        default:
            folly::assume_unreachable();
    }
    if (largestRecvdPacketNum) {
        expectedNextPacketNum = 1 + *largestRecvdPacketNum;
    }
    folly::MutableByteRange initialByteRange(currentPacketData->writableData(), 1);
    folly::MutableByteRange packetNumberByteRange(currentPacketData->writableData() + packetNumberOffset, kMaxPacketNumEncodingSize);

    headerCipher->decryptLongHeader(folly::range(sample), initialByteRange, packetNumberByteRange);
    
    std::pair<PacketNum, size_t> packetNum = parsePacketNumber(initialByteRange.data()[0], packetNumberByteRange, expectedNextPacketNum);
    longHeader.setPacketNumber(packetNum.first);
    BufQueue decryptQueue;
    decryptQueue.append(std::move(currentPacketData));
    size_t aadLen = packetNumberOffset + packetNum.second;
    auto headerData = decryptQueue.splitAtMost(aadLen);
    // parsing verifies that packetLength >= packet number length.
    auto encryptedData = decryptQueue.splitAtMost(parsedLongHeader.packetLength.packetLength - packetNum.second);
    if (!encryptedData) {
        // There should normally be some integrity tag at least in the data,
        // however allowing the aead to process the data even if the tag is not
        // present helps with writing tests.
        encryptedData = folly::IOBuf::create(0);
    }

    Buf decrypted;
    auto decryptAttempt = cipher->tryDecrypt(std::move(encryptedData), headerData.get(), packetNum.first);
    if (!decryptAttempt) {
        return CodecResult(Nothing());
    }
    decrypted = std::move(*decryptAttempt);

    if (!decrypted) {
        // TODO better way of handling this (tests break without this)
        decrypted = folly::IOBuf::create(0);
    }

    return decodeRegularPacket(std::move(longHeader), _params, std::move(decrypted));
}

CodecResult QuicReadCodec::tryParseShortHeaderPacket(Buf data, const AckStates& ackStates, size_t dstConnIdSize, folly::io::Cursor& cursor) {
    // TODO: allow other connid lengths from the state.
    size_t packetNumberOffset = 1 + dstConnIdSize;
    PacketNum expectedNextPacketNum = ackStates.appDataAckState.largestRecvdPacketNum ? (1 + *ackStates.appDataAckState.largestRecvdPacketNum) : 0;
    size_t sampleOffset = packetNumberOffset + kMaxPacketNumEncodingSize;
    Sample sample;
    if (data->computeChainDataLength() < sampleOffset + sample.size()) {
        // There's not enough space for the short header packet
        return CodecResult(Nothing());
    }

    folly::MutableByteRange initialByteRange(data->writableData(), 1);
    folly::MutableByteRange packetNumberByteRange(
        data->writableData() + packetNumberOffset, kMaxPacketNumEncodingSize);
    folly::ByteRange sampleByteRange(
        data->writableData() + sampleOffset, sample.size());

    _oneRttHeaderCipher->decryptShortHeader(
        sampleByteRange, initialByteRange, packetNumberByteRange);
    std::pair<PacketNum, size_t> packetNum = parsePacketNumber(
        initialByteRange.data()[0], packetNumberByteRange, expectedNextPacketNum);
    auto shortHeader =
        parseShortHeader(initialByteRange.data()[0], cursor, dstConnIdSize);
    if (!shortHeader) {
        return CodecResult(Nothing());
    }
    shortHeader->setPacketNumber(packetNum.first);
    if (shortHeader->getProtectionType() == ProtectionType::KeyPhaseOne) {
        return CodecResult(Nothing());
    }

    // We know that the iobuf is not chained. This means that we can safely have a
    // non-owning reference to the header without cloning the buffer. If we don't
    // clone the buffer, the buffer will not show up as shared and we can decrypt
    // in-place.
    size_t aadLen = packetNumberOffset + packetNum.second;
    folly::IOBuf headerData = folly::IOBuf::wrapBufferAsValue(data->data(), aadLen);
    data->trimStart(aadLen);

    Buf decrypted;
    auto decryptAttempt = _oneRttReadCipher->tryDecrypt(std::move(data), &headerData, packetNum.first);
    if (!decryptAttempt) {
        auto protectionType = shortHeader->getProtectionType();
        return CodecResult(Nothing());
    }
    decrypted = std::move(*decryptAttempt);
    if (!decrypted) {
        // TODO better way of handling this (tests break without this)
        decrypted = folly::IOBuf::create(0);
    }

    return decodeRegularPacket(std::move(*shortHeader), _params, std::move(decrypted));
}

CodecResult QuicReadCodec::parsePacket(BufQueue& queue, const AckStates& ackStates, size_t dstConnIdSize) {
    if (queue.empty()) {
        return CodecResult(Nothing());
    }
    folly::io::Cursor cursor(queue.front());
    if (!cursor.canAdvance(sizeof(uint8_t))) {
        return CodecResult(Nothing());
    }
    uint8_t initialByte = cursor.readBE<uint8_t>();
    auto headerForm = getHeaderForm(initialByte);
    if (headerForm == HeaderForm::Long) {
        return parseLongHeaderPacket(queue, ackStates);
    }
    // Missing 1-rtt Cipher is the only case we wouldn't consider reset
    // TODO: support key phase one.
    if (!_oneRttReadCipher || !_oneRttHeaderCipher) {
        return CodecResult(CipherUnavailable(queue.move(), ProtectionType::KeyPhaseZero));
    }

    auto data = queue.move();
    folly::Optional<StatelessResetToken> token;
    if (_nodeType == QuicNodeType::Client && initialByte & ShortHeader::kFixedBitMask) {
        auto dataLength = data->length();
        if (_statelessResetToken && dataLength > sizeof(StatelessResetToken)) {
            const uint8_t* tokenSource = data->data() + (dataLength - sizeof(StatelessResetToken));
            // Only allocate & copy the token if it matches the token we have
            if (fizz::CryptoUtils::equal(folly::ByteRange(tokenSource, sizeof(StatelessResetToken)),
                    folly::ByteRange(_statelessResetToken->data(), sizeof(StatelessResetToken)))) {
                        
                token = StatelessResetToken();
                memcpy(token->data(), tokenSource, token->size());
            }
        }
    }

    auto maybeShortHeaderPacket = tryParseShortHeaderPacket(std::move(data), ackStates, dstConnIdSize, cursor);
    if (token && maybeShortHeaderPacket.nothing()) {
        return StatelessReset(*token);
    }
    return maybeShortHeaderPacket;
}

const Aead* QuicReadCodec::getOneRttReadCipher() const {
    return _oneRttReadCipher.get();
}

const Aead* QuicReadCodec::getZeroRttReadCipher() const {
    return _zeroRttReadCipher.get();
}

const Aead* QuicReadCodec::getHandshakeReadCipher() const {
    return _handshakeReadCipher.get();
}

const folly::Optional<StatelessResetToken>& QuicReadCodec::getStatelessResetToken() const {
    return _statelessResetToken;
}

CodecParameters QuicReadCodec::getCodecParameters() const {
    return _params;
}

void QuicReadCodec::setInitialReadCipher(std::unique_ptr<Aead> initialReadCipher) {
    _initialReadCipher = std::move(initialReadCipher);
}

void QuicReadCodec::setOneRttReadCipher(std::unique_ptr<Aead> oneRttReadCipher) {
    _oneRttReadCipher = std::move(oneRttReadCipher);
}

void QuicReadCodec::setZeroRttReadCipher(
    std::unique_ptr<Aead> zeroRttReadCipher) {
    if (_nodeType == QuicNodeType::Client) {
        throw QuicTransportException("Invalid cipher", TransportErrorCode::INTERNAL_ERROR);
    }
    _zeroRttReadCipher = std::move(zeroRttReadCipher);
}

void QuicReadCodec::setHandshakeReadCipher(std::unique_ptr<Aead> handshakeReadCipher) {
    _handshakeReadCipher = std::move(handshakeReadCipher);
}

void QuicReadCodec::setInitialHeaderCipher(std::unique_ptr<PacketNumberCipher> initialHeaderCipher) {
    _initialHeaderCipher = std::move(initialHeaderCipher);
}

void QuicReadCodec::setOneRttHeaderCipher(std::unique_ptr<PacketNumberCipher> oneRttHeaderCipher) {
    _oneRttHeaderCipher = std::move(oneRttHeaderCipher);
}

void QuicReadCodec::setZeroRttHeaderCipher(std::unique_ptr<PacketNumberCipher> zeroRttHeaderCipher) {
    _zeroRttHeaderCipher = std::move(zeroRttHeaderCipher);
}

void QuicReadCodec::setHandshakeHeaderCipher(std::unique_ptr<PacketNumberCipher> handshakeHeaderCipher) {
    _handshakeHeaderCipher = std::move(handshakeHeaderCipher);
}

void QuicReadCodec::setCodecParameters(CodecParameters params) {
    _params = std::move(params);
}

void QuicReadCodec::setClientConnectionId(ConnectionId connId) {
    _clientConnectionId = connId;
}

void QuicReadCodec::setServerConnectionId(ConnectionId connId) {
    _serverConnectionId = connId;
}

void QuicReadCodec::setStatelessResetToken(StatelessResetToken statelessResetToken) {
    _statelessResetToken = std::move(statelessResetToken);
}

const ConnectionId& QuicReadCodec::getClientConnectionId() const {
    return _clientConnectionId.value();
}

const ConnectionId& QuicReadCodec::getServerConnectionId() const {
    return _serverConnectionId.value();
}

const Aead* QuicReadCodec::getInitialCipher() const {
    return _initialReadCipher.get();
}

const PacketNumberCipher* QuicReadCodec::getInitialHeaderCipher() const {
    return _initialHeaderCipher.get();
}

const PacketNumberCipher* QuicReadCodec::getOneRttHeaderCipher() const {
    return _oneRttHeaderCipher.get();
}

const PacketNumberCipher* QuicReadCodec::getHandshakeHeaderCipher() const {
    return _handshakeHeaderCipher.get();
}

const PacketNumberCipher* QuicReadCodec::getZeroRttHeaderCipher() const {
    return _zeroRttHeaderCipher.get();
}

void QuicReadCodec::onHandshakeDone(TimePoint handshakeDoneTime) {
    if (!_handshakeDoneTime) {
        _handshakeDoneTime = handshakeDoneTime;
    }
}

folly::Optional<TimePoint> QuicReadCodec::getHandshakeDoneTime() {
    return _handshakeDoneTime;
}

std::string QuicReadCodec::connIdToHex() const {
    static ConnectionId zeroConn = zeroConnId();
    const auto& serverId = _serverConnectionId.value_or(zeroConn);
    const auto& clientId = _clientConnectionId.value_or(zeroConn);

    return folly::to<std::string>("server=", serverId.hex(), " ", "client=", clientId.hex());
}

CodecResult::CodecResult(RegularQuicPacket&& regularPacketIn)
    : _type(CodecResult::Type::REGULAR_PACKET) {
    new (&packet) RegularQuicPacket(std::move(regularPacketIn));
}

CodecResult::CodecResult(CipherUnavailable&& cipherUnavailableIn)
    : _type(CodecResult::Type::CIPHER_UNAVAILABLE) {
    new (&cipher) CipherUnavailable(std::move(cipherUnavailableIn));
}

CodecResult::CodecResult(StatelessReset&& statelessResetIn)
    : _type(CodecResult::Type::STATELESS_RESET) {
    new (&reset) StatelessReset(std::move(statelessResetIn));
}

CodecResult::CodecResult(RetryPacket&& retryPacketIn)
    : _type(CodecResult::Type::RETRY) {
    new (&retry) RetryPacket(std::move(retryPacketIn));
}

CodecResult::CodecResult(Nothing&&) : _type(CodecResult::Type::NOTHING) {
    new (&none) Nothing();
}

void CodecResult::destroyCodecResult() {
    switch (_type) {
    case CodecResult::Type::REGULAR_PACKET:
        packet.~RegularQuicPacket();
        break;
    case CodecResult::Type::RETRY:
        retry.~RetryPacket();
        break;
    case CodecResult::Type::CIPHER_UNAVAILABLE:
        cipher.~CipherUnavailable();
        break;
    case CodecResult::Type::STATELESS_RESET:
        reset.~StatelessReset();
        break;
    case CodecResult::Type::NOTHING:
        none.~Nothing();
        break;
    }
}

CodecResult::~CodecResult() {
    destroyCodecResult();
}

CodecResult::CodecResult(CodecResult&& other) noexcept {
    switch (other._type) {
        case CodecResult::Type::REGULAR_PACKET:
            new (&packet) RegularQuicPacket(std::move(other.packet));
            break;
        case CodecResult::Type::RETRY:
            new (&retry) RetryPacket(std::move(other.retry));
            break;
        case CodecResult::Type::CIPHER_UNAVAILABLE:
            new (&cipher) CipherUnavailable(std::move(other.cipher));
            break;
        case CodecResult::Type::STATELESS_RESET:
            new (&reset) StatelessReset(std::move(other.reset));
            break;
        case CodecResult::Type::NOTHING:
            new (&none) Nothing(std::move(other.none));
            break;
    }
    _type = other._type;
}

CodecResult& CodecResult::operator=(CodecResult&& other) noexcept {
    destroyCodecResult();
    switch (other._type) {
        case CodecResult::Type::REGULAR_PACKET:
            new (&packet) RegularQuicPacket(std::move(other.packet));
            break;
        case CodecResult::Type::RETRY:
            new (&retry) RetryPacket(std::move(other.retry));
            break;
        case CodecResult::Type::CIPHER_UNAVAILABLE:
            new (&cipher) CipherUnavailable(std::move(other.cipher));
            break;
        case CodecResult::Type::STATELESS_RESET:
            new (&reset) StatelessReset(std::move(other.reset));
            break;
        case CodecResult::Type::NOTHING:
            new (&none) Nothing(std::move(other.none));
            break;
    }
    _type = other._type;
    return *this;
}

CodecResult::Type CodecResult::type() {
    return _type;
}

RegularQuicPacket* CodecResult::regularPacket() {
    if (_type == CodecResult::Type::REGULAR_PACKET) {
        return &packet;
    }
    return nullptr;
}

CipherUnavailable* CodecResult::cipherUnavailable() {
    if (_type == CodecResult::Type::CIPHER_UNAVAILABLE) {
        return &cipher;
    }
    return nullptr;
}

StatelessReset* CodecResult::statelessReset() {
    if (_type == CodecResult::Type::STATELESS_RESET) {
        return &reset;
    }
    return nullptr;
}

RetryPacket* CodecResult::retryPacket() {
    if (_type == CodecResult::Type::RETRY) {
        return &retry;
    }
    return nullptr;
}

Nothing* CodecResult::nothing() {
    if (_type == CodecResult::Type::NOTHING) {
        return &none;
    }
    return nullptr;
}

} // namespace quic
