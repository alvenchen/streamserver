
#include "parse.hpp"

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
            return QuicFrame(decodeAckFrame(cursor, header, params));
            case FrameType::ACK_ECN:
            return QuicFrame(decodeAckFrameWithECN(cursor, header, params));
            case FrameType::RST_STREAM:
            return QuicFrame(decodeRstStreamFrame(cursor));
            case FrameType::STOP_SENDING:
            return QuicFrame(decodeStopSendingFrame(cursor));
            case FrameType::CRYPTO_FRAME:
            return QuicFrame(decodeCryptoFrame(cursor));
            case FrameType::NEW_TOKEN:
            return QuicFrame(decodeNewTokenFrame(cursor));
            case FrameType::STREAM:
            case FrameType::STREAM_FIN:
            case FrameType::STREAM_LEN:
            case FrameType::STREAM_LEN_FIN:
            case FrameType::STREAM_OFF:
            case FrameType::STREAM_OFF_FIN:
            case FrameType::STREAM_OFF_LEN:
            case FrameType::STREAM_OFF_LEN_FIN:
            consumedQueue = true;
            return QuicFrame(decodeStreamFrame(
            queue,
            StreamTypeField(frameTypeInt->first),
            false /* isGroupFrame */));
            case FrameType::GROUP_STREAM:
            case FrameType::GROUP_STREAM_FIN:
            case FrameType::GROUP_STREAM_LEN:
            case FrameType::GROUP_STREAM_LEN_FIN:
            case FrameType::GROUP_STREAM_OFF:
            case FrameType::GROUP_STREAM_OFF_FIN:
            case FrameType::GROUP_STREAM_OFF_LEN:
            case FrameType::GROUP_STREAM_OFF_LEN_FIN:
            consumedQueue = true;
            return QuicFrame(decodeStreamFrame(
            queue,
            StreamTypeField(frameTypeInt->first),
            true /* isGroupFrame */));
            case FrameType::MAX_DATA:
            return QuicFrame(decodeMaxDataFrame(cursor));
            case FrameType::MAX_STREAM_DATA:
            return QuicFrame(decodeMaxStreamDataFrame(cursor));
            case FrameType::MAX_STREAMS_BIDI:
            return QuicFrame(decodeBiDiMaxStreamsFrame(cursor));
            case FrameType::MAX_STREAMS_UNI:
            return QuicFrame(decodeUniMaxStreamsFrame(cursor));
            case FrameType::DATA_BLOCKED:
            return QuicFrame(decodeDataBlockedFrame(cursor));
            case FrameType::STREAM_DATA_BLOCKED:
            return QuicFrame(decodeStreamDataBlockedFrame(cursor));
            case FrameType::STREAMS_BLOCKED_BIDI:
            return QuicFrame(decodeBiDiStreamsBlockedFrame(cursor));
            case FrameType::STREAMS_BLOCKED_UNI:
            return QuicFrame(decodeUniStreamsBlockedFrame(cursor));
            case FrameType::NEW_CONNECTION_ID:
            return QuicFrame(decodeNewConnectionIdFrame(cursor));
            case FrameType::RETIRE_CONNECTION_ID:
            return QuicFrame(decodeRetireConnectionIdFrame(cursor));
            case FrameType::PATH_CHALLENGE:
            return QuicFrame(decodePathChallengeFrame(cursor));
            case FrameType::PATH_RESPONSE:
            return QuicFrame(decodePathResponseFrame(cursor));
            case FrameType::CONNECTION_CLOSE:
            return QuicFrame(decodeConnectionCloseFrame(cursor));
            case FrameType::CONNECTION_CLOSE_APP_ERR:
            return QuicFrame(decodeApplicationClose(cursor));
            case FrameType::HANDSHAKE_DONE:
            return QuicFrame(decodeHandshakeDoneFrame(cursor));
            case FrameType::DATAGRAM: {
            consumedQueue = true;
            return QuicFrame(decodeDatagramFrame(queue, false /* hasLen */));
            }
            case FrameType::DATAGRAM_LEN: {
            consumedQueue = true;
            return QuicFrame(decodeDatagramFrame(queue, true /* hasLen */));
            }
            case FrameType::KNOB:
            return QuicFrame(decodeKnobFrame(cursor));
            case FrameType::ACK_FREQUENCY:
            return QuicFrame(decodeAckFrequencyFrame(cursor));
            case FrameType::IMMEDIATE_ACK:
            return QuicFrame(decodeImmediateAckFrame(cursor));
            case FrameType::ACK_RECEIVE_TIMESTAMPS:
            auto frame = QuicFrame(decodeAckFrameWithReceivedTimestamps(
            cursor, header, params, FrameType::ACK_RECEIVE_TIMESTAMPS));
            return frame;
            }
        } catch (const std::exception& e) {
            error = true;
            //throw QuicTransportException(fmt::format("Frame format invalid, type={}, error={}",frameTypeInt->first,e.what()),TransportErrorCode::FRAME_ENCODING_ERROR,frameType);
            throw std::runtime_error(fmt::format("Frame format invalid, type={}, error={}",frameTypeInt->first,e.what());
        }
        error = true;

        //throw QuicTransportException(folly::to<std::string>("Unknown frame, type=", frameTypeInt->first),TransportErrorCode::FRAME_ENCODING_ERROR,frameType);
        
}


}