#include "quic_header.hpp"

namespace quic{

HeaderForm getHeaderForm(uint8_t headerValue){
    if (headerValue & kHeaderFormMask) {
        return HeaderForm::Long;
    }
    return HeaderForm::Short;
}


LongHeader::LongHeader(Types type, LongHeaderInvariant invariant, std::string token)
    : _longHeaderType(type), _invariant(std::move(invariant)), _token(std::move(token)) {

}

LongHeader::LongHeader(Types type, const ConnectionId& srcConnId, const ConnectionId& dstConnId, 
    PacketNum packetNum, QuicVersion version, std::string token)
        : _longHeaderType(type), _invariant(LongHeaderInvariant(version, srcConnId, dstConnId)),
        _token(std::move(token)) {
    setPacketNumber(packetNum);
}


LongHeader::Types LongHeader::getHeaderType() const noexcept {
    return _longHeaderType;
}

const ConnectionId& LongHeader::getSourceConnId() const {
    return _invariant.srcConnId;
}

const ConnectionId& LongHeader::getDestinationConnId() const {
    return _invariant.dstConnId;
}

QuicVersion LongHeader::getVersion() const {
    return _invariant.version;
}

bool LongHeader::hasToken() const {
    return !_token.empty();
}

const std::string& LongHeader::getToken() const {
    return _token;
}

void LongHeader::setPacketNumber(PacketNum packetNum) {
    _packetSequenceNum = packetNum;
}

ProtectionType LongHeader::getProtectionType() const {
    return longHeaderTypeToProtectionType(getHeaderType());
}


/*short header*/
ShortHeaderInvariant::ShortHeaderInvariant(ConnectionId dcid)
    : destinationConnId(std::move(dcid)){
}

ShortHeader::ShortHeader(ProtectionType protectionType, ConnectionId connId, PacketNum packetNum)
    : protectionType_(protectionType), connectionId_(std::move(connId)) {
    if (protectionType_ != ProtectionType::KeyPhaseZero &&
        protectionType_ != ProtectionType::KeyPhaseOne) {
        throw std::logic_error("bad short header protection type");
    }
    setPacketNumber(packetNum);
}

ShortHeader::ShortHeader(ProtectionType protectionType, ConnectionId connId)
    : protectionType_(protectionType), connectionId_(std::move(connId)) {
    if (protectionType_ != ProtectionType::KeyPhaseZero &&
        protectionType_ != ProtectionType::KeyPhaseOne) {

        throw std::logic_error("bad short header protection type");
    }
}

ProtectionType ShortHeader::getProtectionType() const {
    return protectionType_;
}

const ConnectionId& ShortHeader::getConnectionId() const {
    return connectionId_;
}

void ShortHeader::setPacketNumber(PacketNum packetNum) {
    packetSequenceNum_ = packetNum;
}



PacketHeader::PacketHeader(ShortHeader&& shortHeaderIn)
    : _headerForm(HeaderForm::Short) {
    new (&shortHeader) ShortHeader(std::move(shortHeaderIn));
}

PacketHeader::PacketHeader(LongHeader&& longHeaderIn)
    : _headerForm(HeaderForm::Long) {
    new (&longHeader) LongHeader(std::move(longHeaderIn));
}

PacketHeader::PacketHeader(const PacketHeader& other)
    : _headerForm(other._headerForm) {
    switch (other._headerForm) {
        case HeaderForm::Long:
            new (&longHeader) LongHeader(other.longHeader);
            break;
        case HeaderForm::Short:
            new (&shortHeader) ShortHeader(other.shortHeader);
            break;
    }
}

PacketHeader::PacketHeader(PacketHeader&& other) noexcept
    : _headerForm(other._headerForm) {
    switch (other._headerForm) {
        case HeaderForm::Long:
            new (&longHeader) LongHeader(std::move(other.longHeader));
            break;
        case HeaderForm::Short:
            new (&shortHeader) ShortHeader(std::move(other.shortHeader));
            break;
    }
}

PacketHeader& PacketHeader::operator=(PacketHeader&& other) noexcept {
    destroyHeader();
    switch (other._headerForm) {
        case HeaderForm::Long:
            new (&longHeader) LongHeader(std::move(other.longHeader));
            break;
        case HeaderForm::Short:
            new (&shortHeader) ShortHeader(std::move(other.shortHeader));
            break;
    }
    _headerForm = other._headerForm;
    return *this;
}

PacketHeader& PacketHeader::operator=(const PacketHeader& other) {
    destroyHeader();
    switch (other._headerForm) {
        case HeaderForm::Long:
            new (&longHeader) LongHeader(other.longHeader);
            break;
        case HeaderForm::Short:
            new (&shortHeader) ShortHeader(other.shortHeader);
            break;
    }
    _headerForm = other._headerForm;
    return *this;
}

PacketHeader::~PacketHeader() {
    destroyHeader();
}

void PacketHeader::destroyHeader() {
    switch (_headerForm) {
        case HeaderForm::Long:
            longHeader.~LongHeader();
            break;
        case HeaderForm::Short:
            shortHeader.~ShortHeader();
            break;
    }
}

LongHeader* PacketHeader::asLong() {
    switch (_headerForm) {
        case HeaderForm::Long:
            return &longHeader;
        case HeaderForm::Short:
            return nullptr;
        default:
            folly::assume_unreachable();
    }
}

ShortHeader* PacketHeader::asShort() {
    switch (_headerForm) {
        case HeaderForm::Long:
            return nullptr;
        case HeaderForm::Short:
            return &shortHeader;
        default:
            folly::assume_unreachable();
    }
}

const LongHeader* PacketHeader::asLong() const {
    switch (_headerForm) {
        case HeaderForm::Long:
            return &longHeader;
        case HeaderForm::Short:
            return nullptr;
        default:
            folly::assume_unreachable();
    }
}

const ShortHeader* PacketHeader::asShort() const {
    switch (_headerForm) {
        case HeaderForm::Long:
            return nullptr;
        case HeaderForm::Short:
            return &shortHeader;
        default:
            folly::assume_unreachable();
    }
}

HeaderForm PacketHeader::getHeaderForm() const {
    return _headerForm;
}

ProtectionType PacketHeader::getProtectionType() const {
    switch (_headerForm) {
        case HeaderForm::Long:
            return longHeader.getProtectionType();
        case HeaderForm::Short:
            return shortHeader.getProtectionType();
        default:
            folly::assume_unreachable();
    }
}


/*
    function
*/
LongHeader::Types parseLongHeaderType(uint8_t initialByte) {
    return static_cast<LongHeader::Types>((initialByte & LongHeader::kPacketTypeMask) >> LongHeader::kTypeShift);
}

folly::Expected<ParsedLongHeaderInvariant, TransportErrorCode> parseLongHeaderInvariant(uint8_t initialByte, folly::io::Cursor& cursor) {
    size_t initialLength = cursor.totalLength();
    if (!cursor.canAdvance(sizeof(QuicVersionType))) {
        return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
    }
    auto version = static_cast<QuicVersion>(cursor.readBE<QuicVersionType>());
    if (!cursor.canAdvance(1)) {
        return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
    }
    uint8_t destConnIdLen = cursor.readBE<uint8_t>();
    if (destConnIdLen > kMaxConnectionIdSize) {
        return folly::makeUnexpected(TransportErrorCode::PROTOCOL_VIOLATION);
    }
    if (!cursor.canAdvance(destConnIdLen)) {
        return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
    }
    ConnectionId destConnId(cursor, destConnIdLen);
    if (!cursor.canAdvance(1)) {
        return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
    }
    uint8_t srcConnIdLen = cursor.readBE<uint8_t>();
    if (srcConnIdLen > kMaxConnectionIdSize) {
        return folly::makeUnexpected(TransportErrorCode::PROTOCOL_VIOLATION);
    }
    if (!cursor.canAdvance(srcConnIdLen)) {
        return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
    }
    ConnectionId srcConnId(cursor, srcConnIdLen);
    size_t currentLength = cursor.totalLength();
    size_t bytesRead = initialLength - currentLength;
 
    return ParsedLongHeaderInvariant(initialByte, LongHeaderInvariant(version, std::move(srcConnId), std::move(destConnId)), bytesRead);
}

folly::Expected<ParsedLongHeader, TransportErrorCode> parseLongHeaderVariants(LongHeader::Types type,
    ParsedLongHeaderInvariant parsedLongHeaderInvariant, folly::io::Cursor& cursor, QuicNodeType nodeType) {
    
    if (type == LongHeader::Types::Retry) {
        // The integrity tag is kRetryIntegrityTagLen bytes in length, and the
        // token must be at least one byte, so the remaining length must
        // be > kRetryIntegrityTagLen.
        if (cursor.totalLength() <= kRetryIntegrityTagLen) {
            return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
        }

        Buf token;
        cursor.clone(token, cursor.totalLength() - kRetryIntegrityTagLen);

        return ParsedLongHeader(LongHeader(type, std::move(parsedLongHeaderInvariant.invariant), 
            token ? token->moveToFbString().toStdString() : std::string()), PacketLength(0, 0));
    }

    // TODO Checking kMinInitialDestinationConnIdLength isn't necessary
    // if this packet is in response to a retry.
    if (type == LongHeader::Types::Initial && nodeType == QuicNodeType::Server &&
        parsedLongHeaderInvariant.invariant.dstConnId.size() < kMinInitialDestinationConnIdLength) {

        return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
    }

    Buf token;
    if (type == LongHeader::Types::Initial) {
        auto tokenLen = decodeQuicInteger(cursor);
        if (!tokenLen) {
            return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
        }
        if (!cursor.canAdvance(tokenLen->first)) {
            return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
        }

        if (tokenLen->first > 0) {
            Buf tokenBuf;
            // If tokenLen > token's actual length then the cursor will throw.
            cursor.clone(tokenBuf, tokenLen->first);
            token = std::move(tokenBuf);
        }
    }
    auto pktLen = decodeQuicInteger(cursor);
    if (!pktLen) {
        return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
    }
    if (!cursor.canAdvance(pktLen->first)) {
        return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
    }
    size_t packetNumLen = parsePacketNumberLength(parsedLongHeaderInvariant.initialByte);
    if (!cursor.canAdvance(packetNumLen)) {
        return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
    }
    if (packetNumLen > kMaxPacketNumEncodingSize) {
        return folly::makeUnexpected(TransportErrorCode::FRAME_ENCODING_ERROR);
    }
    return ParsedLongHeader(LongHeader(type, std::move(parsedLongHeaderInvariant.invariant), 
        token ? token->moveToFbString().toStdString() : std::string()), PacketLength(pktLen->first, pktLen->second));
}


ProtectionType longHeaderTypeToProtectionType(
    LongHeader::Types longHeaderType) {
    switch (longHeaderType) {
        case LongHeader::Types::Initial:
        case LongHeader::Types::Retry:
            return ProtectionType::Initial;
        case LongHeader::Types::Handshake:
            return ProtectionType::Handshake;
        case LongHeader::Types::ZeroRtt:
            return ProtectionType::ZeroRtt;
    }
}

PacketNumberSpace protectionTypeToPacketNumberSpace(ProtectionType protectionType) {
    switch (protectionType) {
        case ProtectionType::Initial:
            return PacketNumberSpace::Initial;
        case ProtectionType::Handshake:
            return PacketNumberSpace::Handshake;
        case ProtectionType::ZeroRtt:
        case ProtectionType::KeyPhaseZero:
        case ProtectionType::KeyPhaseOne:
            return PacketNumberSpace::AppData;
    }
    folly::assume_unreachable();
}

}