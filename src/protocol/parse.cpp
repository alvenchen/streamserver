
#include "parse.hpp"
#include "../common/common.hpp"
#include "quic_exception.h"
#include "../folly/Conv.h"

namespace{
    quic::PacketNum nextAckedPacketGap(quic::PacketNum packetNum, uint64_t gap) {
        // Gap cannot overflow because of the definition of quic integer encoding, so
        // we can just add to gap.
        uint64_t adjustedGap = gap + 2;
        if (packetNum < adjustedGap) {
            throw quic::QuicTransportException("Bad gap", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK);
        }
        return packetNum - adjustedGap;
    }

    quic::PacketNum nextAckedPacketLen(quic::PacketNum packetNum, uint64_t ackBlockLen) {
        // Going to allow 0 as a valid value.
        if (packetNum < ackBlockLen) {
            throw quic::QuicTransportException("Bad block len", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK);
        }
        return packetNum - ackBlockLen;
    }
}

namespace quic{

    QuicFrame parseFrame(BufQueue& queue, const PacketHeader& header, const CodecParameters& params) {
        folly::io::Cursor cursor(queue.front());
        auto frameTypeInt = decodeQuicInteger(cursor);
        if (!frameTypeInt) {
            //throw QuicTransportException("Invalid frame-type field", TransportErrorCode::FRAME_ENCODING_ERROR);
            throw std::runtime_error("Invalid frame-type field");
        }
        queue.trimStart(cursor - queue.front());
        bool consumedQueue = false;
        bool error = false;
        SCOPE_EXIT {
            if (consumedQueue || error) {
                return;
            }
            queue.trimStart(cursor - queue.front());
        };
        cursor.reset(queue.front());
        FrameType frameType = static_cast<FrameType>(frameTypeInt->first);
        try
        {
            switch (frameType) {
            case FrameType::PADDING:
                return QuicFrame(decodePaddingFrame(cursor));
            case FrameType::PING:
                return QuicFrame(decodePingFrame(cursor));
            case FrameType::ACK:
                return_ QuicFrame(decodeAckFrame(cursor, header, params));
            
            }
        } catch (const std::exception& e) {
            error = true;
            //throw QuicTransportException(fmt::format("Frame format invalid, type={}, error={}",frameTypeInt->first,e.what()),TransportErrorCode::FRAME_ENCODING_ERROR,frameType);
            throw std::runtime_error(fmt::format("Frame format invalid, type={}, error={}",frameTypeInt->first,e.what()));
        }
        error = true;

        //throw QuicTransportException(folly::to<std::string>("Unknown frame, type=", frameTypeInt->first),TransportErrorCode::FRAME_ENCODING_ERROR,frameType);
        
    }

    PaddingFrame decodePaddingFrame(folly::io::Cursor& cursor) {
        // we might have multiple padding frames in sequence in the common case.
        // Let's consume all the padding and return 1 padding frame for everything.
        static_assert(static_cast<int>(FrameType::PADDING) == 0, "Padding value is 0");
        folly::ByteRange paddingBytes = cursor.peekBytes();
        if (paddingBytes.size() == 0) {
            return PaddingFrame();
        }
        uint8_t firstByte = paddingBytes.data()[0];
        // While type can be variable length, since PADDING frame is always a 0
        // byte frame, the length of the type should be 1 byte.
        if (static_cast<FrameType>(firstByte) != FrameType::PADDING) {
            return PaddingFrame();
        }
        int ret = memcmp(paddingBytes.data(), paddingBytes.data() + 1, paddingBytes.size() - 1);
        if (ret == 0) {
            cursor.skip(paddingBytes.size());
        }
        return PaddingFrame();
    }

    PingFrame decodePingFrame(folly::io::Cursor&) {
        return PingFrame();
    }

    ReadAckFrame decodeAckFrame(folly::io::Cursor& cursor, const PacketHeader& header, const CodecParameters& params, FrameType frameType) {
        ReadAckFrame frame;
        frame.frameType = frameType;
        auto largestAckedInt = decodeQuicInteger(cursor);
        if (!largestAckedInt) {
            throw QuicTransportException("Bad largest acked", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK);
        }
        auto largestAcked = folly::to<PacketNum>(largestAckedInt->first);
        auto ackDelay = decodeQuicInteger(cursor);
        if (!ackDelay) {
            throw QuicTransportException("Bad ack delay", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK);
        }
        auto additionalAckBlocks = decodeQuicInteger(cursor);
        if (!additionalAckBlocks) {
            throw QuicTransportException("Bad ack block count", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK);
        }
        auto firstAckBlockLen = decodeQuicInteger(cursor);
        if (!firstAckBlockLen) {
            throw QuicTransportException("Bad first block", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK);
        }
        // Using default ack delay for long header packets. Before negotiating
        // and ack delay, the sender has to use something, so they use the default
        // ack delay. To keep it consistent the protocol specifies using the same
        // ack delay for all the long header packets.
        uint8_t ackDelayExponentToUse = (header.getHeaderForm() == HeaderForm::Long) ? kDefaultAckDelayExponent : params.peerAckDelayExponent;
        //DCHECK_LT(ackDelayExponentToUse, sizeof(ackDelay->first) * 8);

        PacketNum currentPacketNum = nextAckedPacketLen(largestAcked, firstAckBlockLen->first);
        frame.largestAcked = largestAcked;

        auto adjustedDelay = convertEncodedDurationToMicroseconds(frameType, ackDelayExponentToUse, ackDelay->first);

        if (UNLIKELY(adjustedDelay > 1000 * 1000 * 1000 /* 1000s */)) {
            //LOG(ERROR) << "Quic recvd long ack delay=" << adjustedDelay << " frame type: " << static_cast<uint64_t>(frameType);
            adjustedDelay = 0;
        }
        frame.ackDelay = std::chrono::microseconds(adjustedDelay);

        frame.ackBlocks.emplace_back(currentPacketNum, largestAcked);
        for (uint64_t numBlocks = 0; numBlocks < additionalAckBlocks->first; ++numBlocks) {
            auto currentGap = decodeQuicInteger(cursor);
            if (!currentGap) {
                throw QuicTransportException("Bad gap", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK);
            }
            auto blockLen = decodeQuicInteger(cursor);
            if (!blockLen) {
                throw QuicTransportException("Bad block len", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK);
            }
            PacketNum nextEndPacket = nextAckedPacketGap(currentPacketNum, currentGap->first);
            currentPacketNum = nextAckedPacketLen(nextEndPacket, blockLen->first);
            // We don't need to add the entry when the block length is zero since we
            // already would have processed it in the previous iteration.
            frame.ackBlocks.emplace_back(currentPacketNum, nextEndPacket);
        }

        return frame;
    }

    uint64_t convertEncodedDurationToMicroseconds(FrameType frameType, uint8_t exponentToUse, uint64_t delay){
        // ackDelayExponentToUse is guaranteed to be less than the size of uint64_t
        uint64_t delayOverflowMask = 0xFFFFFFFFFFFFFFFF;
        uint8_t leftShift = (sizeof(delay) * 8 - exponentToUse);
        //DCHECK_LT(leftShift, sizeof(delayOverflowMask) * 8);
        delayOverflowMask = delayOverflowMask << leftShift;
        if ((delay & delayOverflowMask) != 0) {
            throw QuicTransportException("Decoded delay overflows", quic::TransportErrorCode::FRAME_ENCODING_ERROR, frameType);
        }
        uint64_t adjustedDelay = delay << exponentToUse;
        if (adjustedDelay >
            static_cast<uint64_t>(std::numeric_limits<std::chrono::microseconds::rep>::max())) {
            throw QuicTransportException("Bad delay", quic::TransportErrorCode::FRAME_ENCODING_ERROR, frameType);
        }
        return adjustedDelay;
    }
}