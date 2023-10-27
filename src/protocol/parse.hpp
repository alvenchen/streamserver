#pragma once

#include <folly/Optional.h>
#include <folly/Expected.h>
#include <folly/io/Cursor.h>
#include "../common/common.hpp"
#include "quic_header.hpp"
#include "quic_frame.hpp"
#include "../common/BufUtil.h"
#include "quic_integer.hpp"
#include "quic_type.hpp"
#include "quic_packet.hpp"
#include "quic_packet_num.hpp"
#include "quic_constants.hpp"
#include "quic_connection_id.hpp"
#include "../state/transport_setting.h"

namespace quic{

    /**
     * Connection level parameters needed by the codec to decode the packet
     * successfully.
     */
    struct CodecParameters {
        // This must not be set to zero.
        uint8_t peerAckDelayExponent{kDefaultAckDelayExponent};
        QuicVersion version{QuicVersion::QUIC_V1};
        folly::Optional<AckReceiveTimestampsConfig> maybeAckReceiveTimestampsConfig = folly::none;

        CodecParameters() = default;

        CodecParameters(uint8_t peerAckDelayExponentIn, QuicVersion versionIn, folly::Optional<AckReceiveTimestampsConfig> maybeAckReceiveTimestampsConfigIn)
            : peerAckDelayExponent(peerAckDelayExponentIn), version(versionIn), maybeAckReceiveTimestampsConfig(maybeAckReceiveTimestampsConfigIn) {}

        CodecParameters(uint8_t peerAckDelayExponentIn, QuicVersion versionIn)
            : peerAckDelayExponent(peerAckDelayExponentIn), version(versionIn) {}
    };

    struct ParsedLongHeaderInvariant {
        uint8_t initialByte;
        LongHeaderInvariant invariant;
        size_t invariantLength;

        ParsedLongHeaderInvariant(uint8_t initialByteIn, LongHeaderInvariant headerInvariant, size_t length)
            :initialByte(initialByteIn), invariant(std::move(headerInvariant)), invariantLength(length){
        }
     };

    
    /**
     * Decodes a single regular QUIC packet from the cursor.
     * PacketData represents data from 1 QUIC packet.
     * Throws with a QuicException if the data in the cursor is not a complete QUIC
     * packet or the packet could not be decoded correctly.
     */
    RegularQuicPacket decodeRegularPacket(PacketHeader &&header, const CodecParameters &params, Buf packetData);

    /**
     * Decodes a version negotiation packet. Returns a folly::none, if it cannot
     * decode the packet.
     */
    folly::Optional<VersionNegotiationPacket> decodeVersionNegotiation(const ParsedLongHeaderInvariant& longHeaderInvariant, folly::io::Cursor& cursor);

    QuicFrame parseFrame(BufQueue& queue, const PacketHeader& header, const CodecParameters& params);

    /**
     * The following functions decode frames. They throw an QuicException when error
     * occurs.
     */
    PaddingFrame decodePaddingFrame(folly::io::Cursor& cursor);

    PingFrame decodePingFrame(folly::io::Cursor& cursor);

    ReadAckFrame decodeAckFrame(folly::io::Cursor& cursor, const PacketHeader& header, const CodecParameters& params, FrameType frameType=FrameType::ACK);

    ReadAckFrame decodeAckFrameWithECN(folly::io::Cursor& cursor, const PacketHeader& header, const CodecParameters& params);

    ReadAckFrame decodeAckFrameWithReceivedTimestamps(folly::io::Cursor& cursor, const PacketHeader& header, const CodecParameters& params, FrameType frameType);

    RstStreamFrame decodeRstStreamFrame(folly::io::Cursor& cursor);

    StopSendingFrame decodeStopSendingFrame(folly::io::Cursor& cursor);

    ReadCryptoFrame decodeCryptoFrame(folly::io::Cursor& cursor);

    ReadNewTokenFrame decodeNewTokenFrame(folly::io::Cursor& cursor);

    ReadStreamFrame decodeStreamFrame(BufQueue& queue, StreamTypeField frameTypeField, bool isGroupFrame = false);

    MaxDataFrame decodeMaxDataFrame(folly::io::Cursor& cursor);

    MaxStreamDataFrame decodeMaxStreamDataFrame(folly::io::Cursor& cursor);

    MaxStreamsFrame decodeBiDiMaxStreamsFrame(folly::io::Cursor& cursor);

    MaxStreamsFrame decodeUniMaxStreamsFrame(folly::io::Cursor& cursor);

    DataBlockedFrame decodeDataBlockedFrame(folly::io::Cursor& cursor);

    StreamDataBlockedFrame decodeStreamDataBlockedFrame(folly::io::Cursor& cursor);

    StreamsBlockedFrame decodeBiDiStreamsBlockedFrame(folly::io::Cursor& cursor);

    StreamsBlockedFrame decodeUniStreamsBlockedFrame(folly::io::Cursor& cursor);

    NewConnectionIdFrame decodeNewConnectionIdFrame(folly::io::Cursor& cursor);

    RetireConnectionIdFrame decodeRetireConnectionIdFrame(folly::io::Cursor& cursor);

    PathChallengeFrame decodePathChallengeFrame(folly::io::Cursor& cursor);

    PathResponseFrame decodePathResponseFrame(folly::io::Cursor& cursor);

    ConnectionCloseFrame decodeConnectionCloseFrame(folly::io::Cursor& cursor);

    ConnectionCloseFrame decodeApplicationClose(folly::io::Cursor& cursor);

    HandshakeDoneFrame decodeHandshakeDoneFrame(folly::io::Cursor& cursor);

    DatagramFrame decodeDatagramFrame(BufQueue& queue, bool hasLen);

    KnobFrame decodeKnobFrame(folly::io::Cursor& cursor);

    AckFrequencyFrame decodeAckFrequencyFrame(folly::io::Cursor& cursor);

    ImmediateAckFrame decodeImmediateAckFrame(folly::io::Cursor& cursor);


/*
    internal
*/
    uint64_t convertEncodedDurationToMicroseconds(FrameType frameType, uint8_t exponentToUse, uint64_t delay);


/*
    parse header
*/
    size_t parsePacketNumberLength(uint8_t initialByte);

    /**
     * Returns the packet number and the length of the packet number.
     * packetNumberRange should be kMaxPacketNumEncodingSize size.
     */
    std::pair<PacketNum, size_t> parsePacketNumber(uint8_t initialByte, folly::ByteRange packetNumberRange, PacketNum expectedNextPacketNum);

    folly::Expected<ShortHeaderInvariant, TransportErrorCode> parseShortHeaderInvariants(uint8_t initialByte, folly::io::Cursor& cursor, size_t dstConnIdSize = kDefaultConnectionIdSize);

    folly::Expected<ShortHeader, TransportErrorCode> parseShortHeader(uint8_t initialByte, folly::io::Cursor& cursor, size_t dstConnIdSize = kDefaultConnectionIdSize);

    folly::Expected<ParsedLongHeaderInvariant, TransportErrorCode> parseLongHeaderInvariant(uint8_t initialByte, folly::io::Cursor& cursor);

    // nodeType: Determine if we allow 0-len dst connection ids.
    folly::Expected<ParsedLongHeader, TransportErrorCode> parseLongHeaderVariants(LongHeader::Types type, ParsedLongHeaderInvariant longHeaderInvariant, folly::io::Cursor& cursor, QuicNodeType nodeType = QuicNodeType::Server);

}