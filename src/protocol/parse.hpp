#pragma once

#include "../common/common.hpp"
#include "quic_header.hpp"
#include "quic_frame.hpp"
#include "../common/BufUtil.h"
#include "transport_settings.h"
#include "quic_integer.h"
#include "quic_type.hpp"

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
}