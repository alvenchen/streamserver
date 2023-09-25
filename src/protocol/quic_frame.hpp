#pragma once

#include <optional>
#include "../common/common.hpp"
#include "quic_constants.hpp"
#include "quic_packet_num.hpp"
#include <common/IntervalSet.h>
#include "quic.hpp"
#include <folly/io/IOBuf.h>
#include "quic_exception.h"

namespace quic{
    template <class T>
    using SmallVec = std::vector<T>;

    //Frame type
    struct PaddingFrame {
        // How many contiguous padding frames this represents.
        uint16_t numFrames{1};
        bool operator==(const PaddingFrame& rhs) const {
            return numFrames == rhs.numFrames;
        }
    };

    struct PingFrame {
        PingFrame() = default;
        bool operator==(const PingFrame& /*rhs*/) const {
            return true;
        }
    };

    struct RstStreamFrame {
        StreamId streamId;
        ApplicationErrorCode errorCode;
        uint64_t offset;

        RstStreamFrame(StreamId streamIdIn, ApplicationErrorCode errorCodeIn, uint64_t offsetIn)
            : streamId(streamIdIn), errorCode(errorCodeIn), offset(offsetIn) {}

        bool operator==(const RstStreamFrame& rhs) const {
            return streamId == rhs.streamId && errorCode == rhs.errorCode && offset == rhs.offset;
        }
    };

    struct ImmediateAckFrame {
        ImmediateAckFrame() = default;
        bool operator==(const ImmediateAckFrame& /*rhs*/) const {
            return true;
        }
    };

    struct AckFrequencyFrame {
        uint64_t sequenceNumber; // Used to identify newest.
        uint64_t packetTolerance; // How many packets before ACKing.
        uint64_t updateMaxAckDelay; // New max_ack_delay to use.
        uint64_t reorderThreshold; // New out-of-order packet threshold before ACKing.

        bool operator==(const AckFrequencyFrame& other) const {
            return other.sequenceNumber == sequenceNumber &&
                other.packetTolerance == packetTolerance &&
                other.updateMaxAckDelay == updateMaxAckDelay &&
                other.reorderThreshold == reorderThreshold;
        }
    };

    /**
     * AckBlock represents a series of continuous packet sequences from
     * [startPacket, endPacket]
     */
    struct AckBlock {
        PacketNum startPacket;
        PacketNum endPacket;

        AckBlock(PacketNum start, PacketNum end)
            : startPacket(start), endPacket(end) {}
    };

    struct RecvdPacketsTimestampsRange {
        uint64_t gap;
        uint64_t timestamp_delta_count;
        std::vector<uint64_t> deltas;
    };

    using RecvdPacketsTimestampsRangeVec = std::vector<RecvdPacketsTimestampsRange>;


    /**
     0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                     Largest Acknowledged (i)                ...
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                          ACK Delay (i)                      ...
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                       ACK Block Count (i)                   ...
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                          ACK Blocks (*)                     ...
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                      First ACK Block (i)                    ...
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                             Gap (i)                         ...
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                    Additional ACK Block (i)                 ...
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
    struct ReadAckFrame {
        PacketNum largestAcked;
        std::chrono::microseconds ackDelay{0us};
        // Only true for the special case of packet number space dropping.
        bool implicit{false};
        // Should have at least 1 block.
        // These are ordered in descending order by start packet.
        using Vec = SmallVec<AckBlock>;
        Vec ackBlocks;
        FrameType frameType = FrameType::ACK;
        std::optional<std::chrono::microseconds> maybeLatestRecvdPacketTime;
        std::optional<PacketNum> maybeLatestRecvdPacketNum;
        RecvdPacketsTimestampsRangeVec recvdPacketsTimestampRanges;
        bool operator==(const ReadAckFrame& /*rhs*/) const {
            // Can't compare ackBlocks, function is just here to appease compiler.
            return false;
        }
    };

    struct WriteAckFrame {
        // Since we don't need this to be an IntervalSet, they are stored directly
        // in a vector, in reverse order.
        // TODO should this be a small_vector?
        using AckBlockVec = std::vector<Interval<PacketNum>>;
        AckBlockVec ackBlocks;
        // Delay in sending ack from time that packet was received.
        std::chrono::microseconds ackDelay{0us};
        FrameType frameType = FrameType::ACK;
        std::optional<std::chrono::microseconds> maybeLatestRecvdPacketTime;
        std::optional<PacketNum> maybeLatestRecvdPacketNum;
        RecvdPacketsTimestampsRangeVec recvdPacketsTimestampRanges;
        bool operator==(const WriteAckFrame& /*rhs*/) const {
            // Can't compare ackBlocks, function is just here to appease compiler.
            return false;
        }
    };

    struct StopSendingFrame {
        StreamId streamId;
        ApplicationErrorCode errorCode;

        StopSendingFrame(StreamId streamIdIn, ApplicationErrorCode errorCodeIn)
            : streamId(streamIdIn), errorCode(errorCodeIn) {}
        bool operator==(const StopSendingFrame& rhs) const {
            return streamId == rhs.streamId && errorCode == rhs.errorCode;
        }
    };

    using Buf = std::unique_ptr<folly::IOBuf>;
    struct ReadCryptoFrame {
        uint64_t offset;
        Buf data;

        ReadCryptoFrame(uint64_t offsetIn, Buf dataIn)
            : offset(offsetIn), data(std::move(dataIn)) {}

        explicit ReadCryptoFrame(uint64_t offsetIn)
            : offset(offsetIn), data(folly::IOBuf::create(0)) {}

        // Stuff stored in a variant type needs to be copyable.
        ReadCryptoFrame(const ReadCryptoFrame& other) {
            offset = other.offset;
            if (other.data) {
                data = other.data->clone();
            }
        }

        ReadCryptoFrame(ReadCryptoFrame&& other) noexcept {
            offset = other.offset;
            data = std::move(other.data);
        }

        ReadCryptoFrame& operator=(const ReadCryptoFrame& other) {
            offset = other.offset;
            if (other.data) {
                data = other.data->clone();
            }
            return *this;
        }

        ReadCryptoFrame& operator=(ReadCryptoFrame&& other) {
            offset = other.offset;
            data = std::move(other.data);
            return *this;
        }

        bool operator==(const ReadCryptoFrame& other) const {
            folly::IOBufEqualTo eq;
            return offset == other.offset && eq(data, other.data);
        }
    };

    struct ReadNewTokenFrame {
        Buf token;
        ReadNewTokenFrame(Buf tokenIn) : token(std::move(tokenIn)) {}

        // Stuff stored in a variant type needs to be copyable.
        ReadNewTokenFrame(const ReadNewTokenFrame& other) {
            if (other.token) {
                token = other.token->clone();
            }
        }

        ReadNewTokenFrame& operator=(const ReadNewTokenFrame& other) {
            if (other.token) {
                token = other.token->clone();
            }
            return *this;
        }

        bool operator==(const ReadNewTokenFrame& other) const {
            folly::IOBufEqualTo eq;
            return eq(token, other.token);
        }
    };

    struct ReadStreamFrame {
        StreamId streamId;
        folly::Optional<StreamGroupId> streamGroupId;
        uint64_t offset;
        Buf data;
        bool fin;

        ReadStreamFrame(StreamId streamIdIn, uint64_t offsetIn, Buf dataIn, bool finIn, folly::Optional<StreamGroupId> streamGroupIdIn = folly::none)
            : streamId(streamIdIn), streamGroupId(streamGroupIdIn), offset(offsetIn), data(std::move(dataIn)), fin(finIn) {}

        ReadStreamFrame(StreamId streamIdIn, uint64_t offsetIn, bool finIn, folly::Optional<StreamGroupId> streamGroupIdIn = folly::none)
            : streamId(streamIdIn), streamGroupId(streamGroupIdIn), offset(offsetIn), data(folly::IOBuf::create(0)), fin(finIn) {}

        // Stuff stored in a variant type needs to be copyable.
        ReadStreamFrame(const ReadStreamFrame& other) {
            streamId = other.streamId;
            offset = other.offset;
            if (other.data) {
                data = other.data->clone();
            }
            fin = other.fin;
            streamGroupId = other.streamGroupId;
        }

        ReadStreamFrame(ReadStreamFrame&& other) noexcept {
            streamId = other.streamId;
            offset = other.offset;
            data = std::move(other.data);
            fin = other.fin;
            streamGroupId = other.streamGroupId;
        }

        ReadStreamFrame& operator=(const ReadStreamFrame& other) {
            streamId = other.streamId;
            offset = other.offset;
            if (other.data) {
                data = other.data->clone();
            }
            fin = other.fin;
            streamGroupId = other.streamGroupId;
            return *this;
        }

        ReadStreamFrame& operator=(ReadStreamFrame&& other) {
            streamId = other.streamId;
            offset = other.offset;
            data = std::move(other.data);
            fin = other.fin;
            streamGroupId = other.streamGroupId;
            return *this;
        }

        bool operator==(const ReadStreamFrame& other) const {
            folly::IOBufEqualTo eq;
            return streamId == other.streamId && offset == other.offset &&
                fin == other.fin && eq(data, other.data) &&
                streamGroupId == other.streamGroupId;
        }
    };

    struct MaxDataFrame {
        uint64_t maximumData;
        explicit MaxDataFrame(uint64_t maximumDataIn) : maximumData(maximumDataIn) {}
        bool operator==(const MaxDataFrame& rhs) const {
            return maximumData == rhs.maximumData;
        }
    };

    struct MaxStreamDataFrame {
        StreamId streamId;
        uint64_t maximumData;
        MaxStreamDataFrame(StreamId streamIdIn, uint64_t maximumDataIn)
            : streamId(streamIdIn), maximumData(maximumDataIn) {}
        bool operator==(const MaxStreamDataFrame& rhs) const {
            return streamId == rhs.streamId && maximumData == rhs.maximumData;
        }
    };

    struct DataBlockedFrame {
        // the connection-level limit at which blocking occurred
        uint64_t dataLimit;
        explicit DataBlockedFrame(uint64_t dataLimitIn) : dataLimit(dataLimitIn) {}
        bool operator==(const DataBlockedFrame& rhs) const {
            return dataLimit == rhs.dataLimit;
        }
    };

    struct StreamDataBlockedFrame {
        StreamId streamId;
        uint64_t dataLimit;
        StreamDataBlockedFrame(StreamId streamIdIn, uint64_t dataLimitIn)
            : streamId(streamIdIn), dataLimit(dataLimitIn) {}
        bool operator==(const StreamDataBlockedFrame& rhs) const {
            return streamId == rhs.streamId && dataLimit == rhs.dataLimit;
        }
    };

    struct StreamsBlockedFrame {
        uint64_t streamLimit;
        bool isForBidirectional{false};
        explicit StreamsBlockedFrame(uint64_t streamLimitIn, bool isBidirectionalIn)
            : streamLimit(streamLimitIn), isForBidirectional(isBidirectionalIn) {}
        bool isForBidirectionalStream() const {
            return isForBidirectional;
        }
        bool isForUnidirectionalStream() const {
            return !isForBidirectional;
        }
        bool operator==(const StreamsBlockedFrame& rhs) const {
            return streamLimit == rhs.streamLimit;
        }
    };

    struct ConnectionCloseFrame {
        // Members are not const to allow this to be movable.
        QuicErrorCode errorCode;
        std::string reasonPhrase;
        // Per QUIC specification: type of frame that triggered the (close) error.
        // A value of 0 (PADDING frame) implies the frame type is unknown
        FrameType closingFrameType;

        ConnectionCloseFrame(QuicErrorCode errorCodeIn, std::string reasonPhraseIn, FrameType closingFrameTypeIn = FrameType::PADDING)
            : errorCode(std::move(errorCodeIn)), reasonPhrase(std::move(reasonPhraseIn)), closingFrameType(closingFrameTypeIn) {}

        FrameType getClosingFrameType() const noexcept {
            return closingFrameType;
        }
        bool operator==(const ConnectionCloseFrame& rhs) const {
            return errorCode == rhs.errorCode && reasonPhrase == rhs.reasonPhrase;
        }
    };

    // generic type
    // https://datatracker.ietf.org/doc/html/rfc9000#Frame-Types-and-Formats
    struct QuicFrame{
        enum class TYPE {
            PADDING_FRAME,
            PING_FRAME,
            READ_ACK_FRAME,
            WRITE_ACK_FRAME,
            RST_STREAM_FRAME,
            STOP_SENDING_FRAME,
            READ_CRYPTO_FRAME,
            READ_NEW_TOKEN_FRAME,
            READ_STREAM_FRAME,
            MAX_DATA_FRAME,
            MAX_STREAM_DATA_FRAME,
            IMMEDIATE_ACK_FRAME,
            ACK_FREQUENCY_FRAME,
        };

        ~QuicFrame();
        QuicFrame(QuicFrame&& other) noexcept;
        QuicFrame& operator=(QuicFrame&& other) noexcept;
        QuicFrame(PaddingFrame&& in);
        QuicFrame(PingFrame&& in);
        QuicFrame(ReadAckFrame&& in);
        QuicFrame(WriteAckFrame&& in);
        QuicFrame(RstStreamFrame&& in);
        QuicFrame(StopSendingFrame&& in);
        QuicFrame(ReadCryptoFrame&& in);
        QuicFrame(ReadNewTokenFrame&& in);
        QuicFrame(ReadStreamFrame&& in);
        QuicFrame(MaxDataFrame&& in);
        QuicFrame(MaxStreamDataFrame&& in);
        QuicFrame(ImmediateAckFrame&& in);
        QuicFrame(AckFrequencyFrame&& in);

        TYPE type() const;

        PaddingFrame* paddingFrame();
        PingFrame* pingFrame();
        ReadAckFrame* readAckFrame();
        WriteAckFrame* writeAckFrame();
        RstStreamFrame* rstStreamFrame();
        StopSendingFrame* stopSendingFrame();
        ReadCryptoFrame* readCryptoFrame();
        ReadNewTokenFrame* readNewTokenFrame();
        ReadStreamFrame* readStreamFrame();
        MaxDataFrame* maxDataFrame();
        MaxStreamDataFrame* maxStreamDataFrame();
        ImmediateAckFrame* immediateAckFrame();
        AckFrequencyFrame* ackFrequencyFrame();

    private:
        void destroy() noexcept;

        TYPE _type;
        union{
            PaddingFrame padding;
            PingFrame ping;
            ReadAckFrame readAck;
            WriteAckFrame writeAck;
            RstStreamFrame rst;
            StopSendingFrame stopSend;
            ReadCryptoFrame readCrypto;
            ReadNewTokenFrame readNewToken;
            // TODO NewTokenFrame newToken;
            ReadStreamFrame readStream;
            MaxDataFrame maxData;
            MaxStreamDataFrame maxStreamData;
            // TODO MaxStreamsFrame
            DataBlockedFrame dataBlocked;
            StreamDataBlockedFrame streamDataBlocked;
            StreamsBlockedFrame streamBlocked;
            // TODO NewConnectionIdFrame
            // TODO RetireConnectionIdFrame
            // TODO PathChallengeFrame
            // TODO PathResponseFrame
            ConnectionCloseFrame connClose;
            // TODO HandshakeDoneFrame

            ImmediateAckFrame immAck;
            AckFrequencyFrame ackFrequency;
        };
    };

    

}