
#include "parse.hpp"
#include "../common/common.hpp"

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

}