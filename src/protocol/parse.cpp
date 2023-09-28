
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
                    return QuicFrame(decodeStreamFrame(queue, StreamTypeField(frameTypeInt->first), false /* isGroupFrame */));
                case FrameType::GROUP_STREAM:
                case FrameType::GROUP_STREAM_FIN:
                case FrameType::GROUP_STREAM_LEN:
                case FrameType::GROUP_STREAM_LEN_FIN:
                case FrameType::GROUP_STREAM_OFF:
                case FrameType::GROUP_STREAM_OFF_FIN:
                case FrameType::GROUP_STREAM_OFF_LEN:
                case FrameType::GROUP_STREAM_OFF_LEN_FIN:
                    consumedQueue = true;
                    return QuicFrame(decodeStreamFrame(queue, StreamTypeField(frameTypeInt->first), true /* isGroupFrame */));
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
                    return QuicFrame(decodeAckFrameWithReceivedTimestamps(cursor, header, params, FrameType::ACK_RECEIVE_TIMESTAMPS));
            }
        } catch (const std::exception& e) {
            error = true;
            throw QuicTransportException(fmt::format("Frame format invalid, type={}, error={}",frameTypeInt->first,e.what()),TransportErrorCode::FRAME_ENCODING_ERROR,frameType);
            //throw std::runtime_error(fmt::format("Frame format invalid, type={}, error={}",frameTypeInt->first,e.what()));
        }
        error = true;

        throw QuicTransportException(folly::to<std::string>("Unknown frame, type=", frameTypeInt->first),TransportErrorCode::FRAME_ENCODING_ERROR,frameType);
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

    ReadAckFrame decodeAckFrameWithECN(folly::io::Cursor& cursor, const PacketHeader& header, const CodecParameters& params) {
        // TODO this is incomplete
        auto readAckFrame = decodeAckFrame(cursor, header, params);
        // TODO we simply ignore ECN blocks in ACK-ECN frames for now.
        auto ect_0 = decodeQuicInteger(cursor);
        if (!ect_0) {
            throw QuicTransportException("Bad ECT(0) value", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_ECN);
        }
        auto ect_1 = decodeQuicInteger(cursor);
        if (!ect_1) {
            throw QuicTransportException("Bad ECT(1) value", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_ECN);
        }
        auto ect_ce = decodeQuicInteger(cursor);
        if (!ect_ce) {
            throw QuicTransportException("Bad ECT-CE value", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_ECN);
        }
        return readAckFrame;
    }

    ReadAckFrame decodeAckFrameWithReceivedTimestamps(folly::io::Cursor& cursor, const PacketHeader& header, const CodecParameters& params, FrameType frameType) {
        ReadAckFrame frame;
        frame = decodeAckFrame(cursor, header, params, frameType);

        auto latestRecvdPacketNum = decodeQuicInteger(cursor);
        if (!latestRecvdPacketNum) {
            throw QuicTransportException("Bad latest received packet number", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_RECEIVE_TIMESTAMPS);
        }
        frame.maybeLatestRecvdPacketNum = latestRecvdPacketNum->first;

        auto latestRecvdPacketTimeDelta = decodeQuicInteger(cursor);
        if (!latestRecvdPacketTimeDelta) {
            throw QuicTransportException("Bad receive packet timestamp delta", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_RECEIVE_TIMESTAMPS);
        }
        frame.maybeLatestRecvdPacketTime = std::chrono::microseconds(latestRecvdPacketTimeDelta->first);

        auto timeStampRangeCount = decodeQuicInteger(cursor);
        if (!timeStampRangeCount) {
            throw QuicTransportException("Bad receive timestamps range count", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_RECEIVE_TIMESTAMPS);
        }
        for (uint64_t numRanges = 0; numRanges < timeStampRangeCount->first; numRanges++) {
            RecvdPacketsTimestampsRange timeStampRange;
            auto receiveTimeStampsGap = decodeQuicInteger(cursor);
            if (!receiveTimeStampsGap) {
                throw QuicTransportException("Bad receive timestamps gap", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_RECEIVE_TIMESTAMPS);
            }
            timeStampRange.gap = receiveTimeStampsGap->first;
            auto receiveTimeStampsLen = decodeQuicInteger(cursor);
            if (!receiveTimeStampsLen) {
                throw QuicTransportException("Bad receive timestamps block length", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_RECEIVE_TIMESTAMPS);
            }
            timeStampRange.timestamp_delta_count = receiveTimeStampsLen->first;
            uint8_t receiveTimestampsExponentToUse = (params.maybeAckReceiveTimestampsConfig)
                ? params.maybeAckReceiveTimestampsConfig.value().receiveTimestampsExponent
                : kDefaultReceiveTimestampsExponent;
            for (uint64_t i = 0; i < receiveTimeStampsLen->first; i++) {
                auto delta = decodeQuicInteger(cursor);
                if (!delta) {
                    throw QuicTransportException("Bad receive timestamps delta", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_RECEIVE_TIMESTAMPS);
                }
                //DCHECK_LT(receiveTimestampsExponentToUse, sizeof(delta->first) * 8);
                auto adjustedDelta = convertEncodedDurationToMicroseconds(frameType, receiveTimestampsExponentToUse, delta->first);
                timeStampRange.deltas.push_back(adjustedDelta);
            }
            frame.recvdPacketsTimestampRanges.emplace_back(timeStampRange);
        }
        return frame;
    }

    RstStreamFrame decodeRstStreamFrame(folly::io::Cursor& cursor) {
        auto streamId = decodeQuicInteger(cursor);
        if (!streamId) {
            throw QuicTransportException("Bad streamId", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::RST_STREAM);
        }
        ApplicationErrorCode errorCode;
        auto varCode = decodeQuicInteger(cursor);
        if (varCode) {
            errorCode = static_cast<ApplicationErrorCode>(varCode->first);
        } else {
            throw QuicTransportException("Cannot decode error code", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::RST_STREAM);
        }
        auto offset = decodeQuicInteger(cursor);
        if (!offset) {
            throw QuicTransportException("Bad offset", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::RST_STREAM);
        }
        return RstStreamFrame(folly::to<StreamId>(streamId->first), errorCode, offset->first);
    }

    StopSendingFrame decodeStopSendingFrame(folly::io::Cursor& cursor) {
        auto streamId = decodeQuicInteger(cursor);
        if (!streamId) {
            throw QuicTransportException("Bad streamId", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::STOP_SENDING);
        }
        ApplicationErrorCode errorCode;
        auto varCode = decodeQuicInteger(cursor);
        if (varCode) {
            errorCode = static_cast<ApplicationErrorCode>(varCode->first);
        } else {
            throw QuicTransportException("Cannot decode error code", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::STOP_SENDING);
        }
        return StopSendingFrame(folly::to<StreamId>(streamId->first), errorCode);
    }

    ReadCryptoFrame decodeCryptoFrame(folly::io::Cursor& cursor) {
        auto optionalOffset = decodeQuicInteger(cursor);
        if (!optionalOffset) {
            throw QuicTransportException("Invalid offset", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::CRYPTO_FRAME);
        }
        uint64_t offset = optionalOffset->first;

        auto dataLength = decodeQuicInteger(cursor);
        if (!dataLength) {
            throw QuicTransportException("Invalid length", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::CRYPTO_FRAME);
        }
        Buf data;
        if (cursor.totalLength() < dataLength->first) {
            throw QuicTransportException("Length mismatch", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::CRYPTO_FRAME);
        }
        // If dataLength > data's actual length then the cursor will throw.
        cursor.clone(data, dataLength->first);
        return ReadCryptoFrame(offset, std::move(data));
    }

    ReadNewTokenFrame decodeNewTokenFrame(folly::io::Cursor& cursor) {
        auto tokenLength = decodeQuicInteger(cursor);
        if (!tokenLength) {
            throw QuicTransportException("Invalid length", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::NEW_TOKEN);
        }
        Buf token;
        if (cursor.totalLength() < tokenLength->first) {
            throw QuicTransportException("Length mismatch", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::NEW_TOKEN);
        }
        // If tokenLength > token's actual length then the cursor will throw.
        cursor.clone(token, tokenLength->first);
        return ReadNewTokenFrame(std::move(token));
    }

    
    ReadStreamFrame decodeStreamFrame(BufQueue& queue, StreamTypeField frameTypeField, bool isGroupFrame) {
        const quic::FrameType frameType = isGroupFrame ? quic::FrameType::GROUP_STREAM : quic::FrameType::STREAM;
        folly::io::Cursor cursor(queue.front());

        auto streamId = decodeQuicInteger(cursor);
        if (!streamId) {
            throw QuicTransportException("Invalid stream id", quic::TransportErrorCode::FRAME_ENCODING_ERROR, frameType);
        }

        folly::Optional<StreamGroupId> groupId;
        if (isGroupFrame) {
            auto gId = decodeQuicInteger(cursor);
            if (!gId) {
                throw QuicTransportException("Invalid group stream id", quic::TransportErrorCode::FRAME_ENCODING_ERROR, frameType);
            }
            groupId = gId->first;
        }

        uint64_t offset = 0;
        if (frameTypeField.hasOffset()) {
            auto optionalOffset = decodeQuicInteger(cursor);
            if (!optionalOffset) {
                throw QuicTransportException("Invalid offset", quic::TransportErrorCode::FRAME_ENCODING_ERROR, frameType);
            }
            offset = optionalOffset->first;
        }
        auto fin = frameTypeField.hasFin();
        folly::Optional<std::pair<uint64_t, size_t>> dataLength;
        if (frameTypeField.hasDataLength()) {
            dataLength = decodeQuicInteger(cursor);
            if (!dataLength) {
                throw QuicTransportException("Invalid length", quic::TransportErrorCode::FRAME_ENCODING_ERROR, frameType);
            }
        }
        Buf data;
        if (dataLength.has_value()) {
            if (cursor.totalLength() < dataLength->first) {
                throw QuicTransportException("Length mismatch", quic::TransportErrorCode::FRAME_ENCODING_ERROR, frameType);
            }
            // If dataLength > data's actual length then the cursor will throw.
            queue.trimStart(cursor - queue.front());
            data = queue.splitAtMost(dataLength->first);
        } else {
            // Missing Data Length field doesn't mean no data. It means the rest of the
            // frame are all data.
            queue.trimStart(cursor - queue.front());
            data = queue.move();
        }
        return ReadStreamFrame(folly::to<StreamId>(streamId->first), offset, std::move(data), fin, groupId);
    }

    MaxDataFrame decodeMaxDataFrame(folly::io::Cursor& cursor) {
        auto maximumData = decodeQuicInteger(cursor);
        if (!maximumData) {
            throw QuicTransportException("Bad Max Data", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::MAX_DATA);
        }
        return MaxDataFrame(maximumData->first);
    }

    MaxStreamDataFrame decodeMaxStreamDataFrame(folly::io::Cursor& cursor) {
        auto streamId = decodeQuicInteger(cursor);
        if (!streamId) {
        throw QuicTransportException("Invalid streamId", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::MAX_STREAM_DATA);
        }
        auto offset = decodeQuicInteger(cursor);
        if (!offset) {
            throw QuicTransportException("Invalid offset", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::MAX_STREAM_DATA);
        }
        return MaxStreamDataFrame(folly::to<StreamId>(streamId->first), offset->first);
    }

    MaxStreamsFrame decodeBiDiMaxStreamsFrame(folly::io::Cursor& cursor) {
        auto streamCount = decodeQuicInteger(cursor);
        if (!streamCount || streamCount->first > kMaxMaxStreams) {
            throw QuicTransportException("Invalid Bi-directional streamId", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::MAX_STREAMS_BIDI);
        }
        return MaxStreamsFrame(streamCount->first, true /* isBidirectional*/);
    }

    MaxStreamsFrame decodeUniMaxStreamsFrame(folly::io::Cursor& cursor) {
        auto streamCount = decodeQuicInteger(cursor);
        if (!streamCount || streamCount->first > kMaxMaxStreams) {
            throw QuicTransportException("Invalid Uni-directional streamId", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::MAX_STREAMS_UNI);
        }
        return MaxStreamsFrame(streamCount->first, false /* isUnidirectional */);
    }

    DataBlockedFrame decodeDataBlockedFrame(folly::io::Cursor& cursor) {
        auto dataLimit = decodeQuicInteger(cursor);
        if (!dataLimit) {
            throw QuicTransportException("Bad offset", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::DATA_BLOCKED);
        }
        return DataBlockedFrame(dataLimit->first);
    }

    StreamDataBlockedFrame decodeStreamDataBlockedFrame(folly::io::Cursor& cursor) {
        auto streamId = decodeQuicInteger(cursor);
        if (!streamId) {
            throw QuicTransportException("Bad streamId", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::STREAM_DATA_BLOCKED);
        }
        auto dataLimit = decodeQuicInteger(cursor);
        if (!dataLimit) {
            throw QuicTransportException("Bad offset", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::STREAM_DATA_BLOCKED);
        }
        return StreamDataBlockedFrame(folly::to<StreamId>(streamId->first), dataLimit->first);
    }

    StreamsBlockedFrame decodeBiDiStreamsBlockedFrame(folly::io::Cursor& cursor) {
        auto streamId = decodeQuicInteger(cursor);
        if (!streamId) {
            throw QuicTransportException("Bad Bi-Directional streamId", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::STREAMS_BLOCKED_BIDI);
        }
        return StreamsBlockedFrame(folly::to<StreamId>(streamId->first), true /* isBidirectional */);
    }

    StreamsBlockedFrame decodeUniStreamsBlockedFrame(folly::io::Cursor& cursor) {
        auto streamId = decodeQuicInteger(cursor);
        if (!streamId) {
            throw QuicTransportException("Bad Uni-direcitonal streamId", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::STREAMS_BLOCKED_UNI);
        }
        return StreamsBlockedFrame(folly::to<StreamId>(streamId->first), false /* isBidirectional */);
    }


    NewConnectionIdFrame decodeNewConnectionIdFrame(folly::io::Cursor& cursor) {
        auto sequenceNumber = decodeQuicInteger(cursor);
        if (!sequenceNumber) {
            throw QuicTransportException("Bad sequence", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::NEW_CONNECTION_ID);
        }
        auto retirePriorTo = decodeQuicInteger(cursor);
        if (!retirePriorTo) {
            throw QuicTransportException("Bad retire prior to", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::NEW_CONNECTION_ID);
        }
        if (!cursor.canAdvance(sizeof(uint8_t))) {
            throw QuicTransportException("Not enough input bytes to read Dest. ConnectionId", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::NEW_CONNECTION_ID);
        }
        auto connIdLen = cursor.readBE<uint8_t>();
        if (cursor.totalLength() < connIdLen) {
            throw QuicTransportException("Bad connid", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::NEW_CONNECTION_ID);
        }
        if (connIdLen > kMaxConnectionIdSize) {
            throw QuicTransportException("ConnectionId invalid length", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::NEW_CONNECTION_ID);
        }
        ConnectionId connId(cursor, connIdLen);
        StatelessResetToken statelessResetToken;
        cursor.pull(statelessResetToken.data(), statelessResetToken.size());
        return NewConnectionIdFrame(sequenceNumber->first, retirePriorTo->first, std::move(connId), std::move(statelessResetToken));
    }

    RetireConnectionIdFrame decodeRetireConnectionIdFrame(folly::io::Cursor& cursor) {
        auto sequenceNum = decodeQuicInteger(cursor);
        if (!sequenceNum) {
            // TODO change the error code
            throw QuicTransportException("Bad sequence num", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::RETIRE_CONNECTION_ID);
        }
        return RetireConnectionIdFrame(sequenceNum->first);
    }

    PathChallengeFrame decodePathChallengeFrame(folly::io::Cursor& cursor) {
        // just parse and ignore expected data
        // A PATH_CHALLENGE frame contains 8 bytes
        if (!cursor.canAdvance(sizeof(uint64_t))) {
            throw QuicTransportException("Not enough input bytes to read path challenge frame.", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::PATH_CHALLENGE);
        }
        auto pathData = cursor.readBE<uint64_t>();
        return PathChallengeFrame(pathData);
    }

    PathResponseFrame decodePathResponseFrame(folly::io::Cursor& cursor) {
        // just parse and ignore expected data
        // Its format is identical to the PATH_CHALLENGE frame
        if (!cursor.canAdvance(sizeof(uint64_t))) {
            throw QuicTransportException("Not enough input bytes to read path response frame.", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::PATH_RESPONSE);
        }
        auto pathData = cursor.readBE<uint64_t>();
        return PathResponseFrame(pathData);
    }

    ConnectionCloseFrame decodeConnectionCloseFrame(folly::io::Cursor& cursor) {
        TransportErrorCode errorCode{};
        auto varCode = decodeQuicInteger(cursor);
        if (varCode) {
            errorCode = static_cast<TransportErrorCode>(varCode->first);
        } else {
            throw QuicTransportException("Failed to parse error code.", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::CONNECTION_CLOSE);
        }
        auto frameTypeField = decodeQuicInteger(cursor);
        if (!frameTypeField || frameTypeField->second != sizeof(uint8_t)) {
            throw QuicTransportException("Bad connection close triggering frame type value", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::CONNECTION_CLOSE);
        }
        FrameType triggeringFrameType = static_cast<FrameType>(frameTypeField->first);
        auto reasonPhraseLength = decodeQuicInteger(cursor);
        if (!reasonPhraseLength || reasonPhraseLength->first > kMaxReasonPhraseLength) {
            throw QuicTransportException("Bad reason phrase length", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::CONNECTION_CLOSE);
        }
        auto reasonPhrase = cursor.readFixedString(folly::to<size_t>(reasonPhraseLength->first));
        return ConnectionCloseFrame(QuicErrorCode(errorCode), std::move(reasonPhrase), triggeringFrameType);
    }

    ConnectionCloseFrame decodeApplicationClose(folly::io::Cursor& cursor) {
        ApplicationErrorCode errorCode{};
        auto varCode = decodeQuicInteger(cursor);
        if (varCode) {
            errorCode = static_cast<ApplicationErrorCode>(varCode->first);
        } else {
            throw QuicTransportException("Failed to parse error code.", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::CONNECTION_CLOSE_APP_ERR);
        }

        auto reasonPhraseLength = decodeQuicInteger(cursor);
        if (!reasonPhraseLength || reasonPhraseLength->first > kMaxReasonPhraseLength) {
            throw QuicTransportException("Bad reason phrase length", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::CONNECTION_CLOSE_APP_ERR);
        }

        auto reasonPhrase = cursor.readFixedString(folly::to<size_t>(reasonPhraseLength->first));
        return ConnectionCloseFrame(QuicErrorCode(errorCode), std::move(reasonPhrase));
    }

    HandshakeDoneFrame decodeHandshakeDoneFrame(folly::io::Cursor& /*cursor*/) {
        return HandshakeDoneFrame();
    }

    DatagramFrame decodeDatagramFrame(BufQueue& queue, bool hasLen) {
        folly::io::Cursor cursor(queue.front());
        size_t length = cursor.length();
        if (hasLen) {
            auto decodeLength = decodeQuicInteger(cursor);
            if (!decodeLength) {
                throw QuicTransportException("Invalid datagram len", TransportErrorCode::FRAME_ENCODING_ERROR, FrameType::DATAGRAM_LEN);
            }
            length = decodeLength->first;
            if (cursor.length() < length) {
                throw QuicTransportException("Invalid datagram frame", TransportErrorCode::FRAME_ENCODING_ERROR, FrameType::DATAGRAM_LEN);
            }
            queue.trimStart(decodeLength->second);
        }
        return DatagramFrame(length, queue.splitAtMost(length));
    }

    KnobFrame decodeKnobFrame(folly::io::Cursor& cursor) {
        auto knobSpace = decodeQuicInteger(cursor);
        if (!knobSpace) {
            throw QuicTransportException("Bad knob space", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::KNOB);
        }
        auto knobId = decodeQuicInteger(cursor);
        if (!knobId) {
            throw QuicTransportException("Bad knob id", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::KNOB);
        }
        auto knobLen = decodeQuicInteger(cursor);
        if (!knobLen) {
            throw QuicTransportException("Bad knob len", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::KNOB);
        }
        Buf knobBlob;
        cursor.cloneAtMost(knobBlob, knobLen->first);
        return KnobFrame(knobSpace->first, knobId->first, std::move(knobBlob));
    }

    AckFrequencyFrame decodeAckFrequencyFrame(folly::io::Cursor& cursor) {
        auto sequenceNumber = decodeQuicInteger(cursor);
        if (!sequenceNumber) {
            throw QuicTransportException("Bad sequence number", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_FREQUENCY);
        }
        auto packetTolerance = decodeQuicInteger(cursor);
        if (!packetTolerance) {
            throw QuicTransportException("Bad packet tolerance", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_FREQUENCY);
        }
        auto updateMaxAckDelay = decodeQuicInteger(cursor);
        if (!updateMaxAckDelay) {
            throw QuicTransportException("Bad update max ack delay", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_FREQUENCY);
        }
        auto reorderThreshold = decodeQuicInteger(cursor);
        if (!reorderThreshold) {
            throw QuicTransportException("Bad reorder threshold", quic::TransportErrorCode::FRAME_ENCODING_ERROR, quic::FrameType::ACK_FREQUENCY);
        }

        AckFrequencyFrame frame;
        frame.sequenceNumber = sequenceNumber->first;
        frame.packetTolerance = packetTolerance->first;
        frame.updateMaxAckDelay = updateMaxAckDelay->first;
        frame.reorderThreshold = reorderThreshold->first;
        return frame;
    }

    ImmediateAckFrame decodeImmediateAckFrame(folly::io::Cursor&) {
       return ImmediateAckFrame();
    }
/*
    internal
*/
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