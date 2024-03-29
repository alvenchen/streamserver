/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <folly/Optional.h>
#include "quic_connection_id.hpp"
#include "quic_packet_num.hpp"
#include "quic_header.hpp"
#include "quic_integer.hpp"
#include "quic_packet.hpp"
#include "quic.hpp"
#include "quic_type.hpp"

#include "quic_packet_num_cipher.hpp"
#include "common/BufUtil.h"
#include "handshake/Aead.hpp"
#include "state/ack_states.h"
#include "parse.hpp"

namespace quic {

/**
 * Structure which describes data which could not be processed by
 * the read codec due to the required cipher being unavailable. The caller might
 * use this to retry later once the cipher is available.
 */
struct CipherUnavailable {
    Buf packet;
    ProtectionType protectionType;

    CipherUnavailable(Buf packetIn, ProtectionType protectionTypeIn)
        : packet(std::move(packetIn)), protectionType(protectionTypeIn) {}
};

/**
 * A type which represents no data.
 */
struct Nothing {};

struct CodecResult {
    enum class Type {
        REGULAR_PACKET,
        RETRY,
        CIPHER_UNAVAILABLE,
        STATELESS_RESET,
        NOTHING
    };

    ~CodecResult();

    CodecResult(CodecResult&& other) noexcept;
    CodecResult& operator=(CodecResult&& other) noexcept;

    /* implicit */ CodecResult(RegularQuicPacket&& regularPacketIn);
    /* implicit */ CodecResult(CipherUnavailable&& cipherUnavailableIn);
    /* implicit */ CodecResult(StatelessReset&& statelessReset);
    /* implicit */ CodecResult(RetryPacket&& retryPacket);
    /* implicit */ CodecResult(Nothing&& nothing);

    Type type();
    RegularQuicPacket* regularPacket();
    CipherUnavailable* cipherUnavailable();
    StatelessReset* statelessReset();
    RetryPacket* retryPacket();
    Nothing* nothing();

private:
    void destroyCodecResult();

    union {
        RegularQuicPacket packet;
        RetryPacket retry;
        CipherUnavailable cipher;
        StatelessReset reset;
        Nothing none;
    };

    Type _type;
};

/**
 * Reads given data and returns parsed long header.
 * Returns an error if parsing is unsuccessful.
 */
folly::Expected<ParsedLongHeader, TransportErrorCode> tryParseLongHeader(folly::io::Cursor& cursor, QuicNodeType nodeType);
folly::Expected<ParsedLongHeader, TransportErrorCode> tryParseLongHeader(const char* buf, size_t &offset, size_t len, QuicNodeType nodeType);

class QuicReadCodec {
public:
    virtual ~QuicReadCodec() = default;

    explicit QuicReadCodec(QuicNodeType nodeType);

    /**
     * Tries to parse a packet from the buffer data.
     * If it is able to parse the packet, then it returns
     * a valid QUIC packet. If it is not able to parse a packet it might return a
     * cipher unavailable structure. The caller can then retry when the cipher is
     * available. A client should call tryParsingVersionNegotiation
     * before the version is negotiated to detect VN.
     */
    virtual CodecResult parsePacket(BufQueue& queue, const AckStates& ackStates, size_t dstConnIdSize = kDefaultConnectionIdSize);
    virtual CodecResult parsePacket(const char* buf, size_t len, const AckStates& ackStates, size_t dstConnIdSize = kDefaultConnectionIdSize);

    /**
     * Tries to parse the packet and returns whether or not
     * it is a version negotiation packet.
     * This returns folly::none if the packet is either not
     * a VN packet or is invalid.
     */
    folly::Optional<VersionNegotiationPacket> tryParsingVersionNegotiation(BufQueue& queue);

    const Aead* getOneRttReadCipher() const;
    const Aead* getZeroRttReadCipher() const;
    const Aead* getHandshakeReadCipher() const;

    const Aead* getInitialCipher() const;

    const PacketNumberCipher* getInitialHeaderCipher() const;
    const PacketNumberCipher* getOneRttHeaderCipher() const;
    const PacketNumberCipher* getHandshakeHeaderCipher() const;
    const PacketNumberCipher* getZeroRttHeaderCipher() const;

    const folly::Optional<StatelessResetToken>& getStatelessResetToken() const;

    CodecParameters getCodecParameters() const;

    void setInitialReadCipher(std::unique_ptr<Aead> initialReadCipher);
    void setOneRttReadCipher(std::unique_ptr<Aead> oneRttReadCipher);
    void setZeroRttReadCipher(std::unique_ptr<Aead> zeroRttReadCipher);
    void setHandshakeReadCipher(std::unique_ptr<Aead> handshakeReadCipher);

    void setInitialHeaderCipher(std::unique_ptr<PacketNumberCipher> initialHeaderCipher);
    void setOneRttHeaderCipher(std::unique_ptr<PacketNumberCipher> oneRttHeaderCipher);
    void setZeroRttHeaderCipher(std::unique_ptr<PacketNumberCipher> zeroRttHeaderCipher);
    void setHandshakeHeaderCipher(std::unique_ptr<PacketNumberCipher> handshakeHeaderCipher);

    void setCodecParameters(CodecParameters params);
    void setClientConnectionId(ConnectionId connId);
    void setServerConnectionId(ConnectionId connId);
    void setStatelessResetToken(StatelessResetToken statelessResetToken);
    const ConnectionId& getClientConnectionId() const;
    const ConnectionId& getServerConnectionId() const;

    /**
     * Should be invoked when the state machine believes that the handshake is
     * complete.
     */
    void onHandshakeDone(TimePoint handshakeDoneTime);

    folly::Optional<TimePoint> getHandshakeDoneTime();

private:
    CodecResult tryParseShortHeaderPacket(Buf data, const AckStates& ackStates, size_t dstConnIdSize, folly::io::Cursor& cursor);
    CodecResult parseLongHeaderPacket(BufQueue& queue, const AckStates& ackStates);
    CodecResult parseLongHeaderPacket(const char* buf, size_t &offset, size_t len , const AckStates& ackStates);

    [[nodiscard]] std::string connIdToHex() const;

    QuicNodeType _nodeType;

    CodecParameters _params;
    folly::Optional<ConnectionId> _clientConnectionId;
    folly::Optional<ConnectionId> _serverConnectionId;

    // Cipher used to decrypt handshake packets.
    std::unique_ptr<Aead> _initialReadCipher;

    std::unique_ptr<Aead> _oneRttReadCipher;
    std::unique_ptr<Aead> _zeroRttReadCipher;
    std::unique_ptr<Aead> _handshakeReadCipher;

    std::unique_ptr<PacketNumberCipher> _initialHeaderCipher;
    std::unique_ptr<PacketNumberCipher> _oneRttHeaderCipher;
    std::unique_ptr<PacketNumberCipher> _zeroRttHeaderCipher;
    std::unique_ptr<PacketNumberCipher> _handshakeHeaderCipher;

    folly::Optional<StatelessResetToken> _statelessResetToken;
    folly::Optional<TimePoint> _handshakeDoneTime;
};

} // namespace quic
