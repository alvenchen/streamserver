#pragma once

#include "../common/common.hpp"
#include "quic_header.hpp"
#include "quic_frame.hpp"
#include "../common/BufUtil.h"
#include "transport_settings.h"
#include "quic_integer.h"

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
    PaddingFrame decodePaddingFrame(folly::io::Cursor&);

    PingFrame decodePingFrame(folly::io::Cursor& cursor);

    ReadAckFrame decodeAckFrame(folly::io::Cursor& cursor, const PacketHeader& header, const CodecParameters& params, FrameType frameType=FrameType::ACK);
}