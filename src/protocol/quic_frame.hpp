#pragma once

#include <optional>
#include "../common/common.hpp"
#include "quic_constants.hpp"
#include "quic_packet_num.hpp"
#include <common/IntervalSet.h>
#include "quic.hpp"
#include "quic_exception.h"
#include "quic_connection_id.hpp"
#include "../common/BufUtil.h"
#include "../state/ack_states.h"

namespace quic{

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
        using Vec = SmallVec<AckBlock, 32>;
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

    struct WriteCryptoFrame {
        uint64_t offset;
        uint64_t len;

        WriteCryptoFrame(uint64_t offsetIn, uint64_t lenIn)
            : offset(offsetIn), len(lenIn) {}

        bool operator==(const WriteCryptoFrame& rhs) const {
            return offset == rhs.offset && len == rhs.len;
        }
    };

    struct NewTokenFrame {
        Buf token;
        explicit NewTokenFrame(Buf tokenIn) : token(std::move(tokenIn)) {}

        NewTokenFrame(const NewTokenFrame& other) {
            if (other.token) {
                token = other.token->clone();
            }
        }
        bool operator==(const NewTokenFrame& rhs) const {
            folly::IOBufEqualTo eq;
            return eq(token, rhs.token);
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

    struct WriteStreamFrame {
        StreamId streamId;
        folly::Optional<StreamGroupId> streamGroupId;
        uint64_t offset;
        uint64_t len;
        bool fin;

        // Whether this WriteStreamFrame is created from a BufMeta, instead of real
        // write buffer data.
        bool fromBufMeta{false};

        uint64_t streamPacketIdx{0};

        WriteStreamFrame(StreamId streamIdIn, uint64_t offsetIn, uint64_t lenIn, 
            bool finIn, bool fromBufMetaIn = false,  folly::Optional<StreamGroupId> streamGroupIdIn = folly::none, 
            uint64_t streamPacketIdxIn = 0)
                : streamId(streamIdIn), streamGroupId(streamGroupIdIn), offset(offsetIn), len(lenIn),
                fin(finIn), fromBufMeta(fromBufMetaIn), streamPacketIdx(streamPacketIdxIn) {
        }

        bool operator==(const WriteStreamFrame& rhs) const {
        return streamId == rhs.streamId && offset == rhs.offset && len == rhs.len &&
            fin == rhs.fin && fromBufMeta == rhs.fromBufMeta &&
            streamGroupId == rhs.streamGroupId &&
            streamPacketIdx == rhs.streamPacketIdx;
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

    struct MaxStreamsFrame {
        // A count of the cumulative number of streams
        uint64_t maxStreams;
        bool isForBidirectional{false};

        explicit MaxStreamsFrame(uint64_t maxStreamsIn, bool isBidirectionalIn)
            : maxStreams(maxStreamsIn), isForBidirectional(isBidirectionalIn) {}

        bool isForBidirectionalStream() const {
            return isForBidirectional;
        }
        bool isForUnidirectionalStream() {
            return !isForBidirectional;
        }
        bool operator==(const MaxStreamsFrame& rhs) const {
            return maxStreams == rhs.maxStreams && isForBidirectional == rhs.isForBidirectional;
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

    struct NewConnectionIdFrame {
        uint64_t sequenceNumber;
        uint64_t retirePriorTo;
        ConnectionId connectionId;
        StatelessResetToken token;

        NewConnectionIdFrame(uint64_t sequenceNumberIn, uint64_t retirePriorToIn, ConnectionId connectionIdIn,
            StatelessResetToken tokenIn)
            : sequenceNumber(sequenceNumberIn), retirePriorTo(retirePriorToIn), connectionId(connectionIdIn),
                token(std::move(tokenIn)) {}

        bool operator==(const NewConnectionIdFrame& rhs) const {
            return sequenceNumber == rhs.sequenceNumber && retirePriorTo == rhs.retirePriorTo &&
                connectionId == rhs.connectionId && token == rhs.token;
        }
    };

    struct RetireConnectionIdFrame {
        uint64_t sequenceNumber;
        explicit RetireConnectionIdFrame(uint64_t sequenceNumberIn) : sequenceNumber(sequenceNumberIn) {}

        bool operator==(const RetireConnectionIdFrame& rhs) const {
            return sequenceNumber == rhs.sequenceNumber;
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

    struct PathChallengeFrame {
        uint64_t pathData;

        explicit PathChallengeFrame(uint64_t pathDataIn) : pathData(pathDataIn) {}

        bool operator==(const PathChallengeFrame& rhs) const {
            return pathData == rhs.pathData;
        }

        bool operator!=(const PathChallengeFrame& rhs) const {
            return !(*this == rhs);
        }
    };

    struct PathResponseFrame {
        uint64_t pathData;

        explicit PathResponseFrame(uint64_t pathDataIn) : pathData(pathDataIn) {}

        bool operator==(const PathResponseFrame& rhs) const {
            return pathData == rhs.pathData;
        }
    };

    struct HandshakeDoneFrame {
        bool operator==(const HandshakeDoneFrame& /*rhs*/) const {
            return true;
        }
    };

    struct DatagramFrame {
        size_t length;
        BufQueue data;

        explicit DatagramFrame(size_t len, Buf buf)
            : length(len), data(std::move(buf)) {
        }

        // Variant requirement:
        DatagramFrame(const DatagramFrame& other)
            : length(other.length),
            data(other.data.front() ? other.data.front()->clone() : nullptr) {
        }

        bool operator==(const DatagramFrame& other) const {
            if (length != other.length) {
                return false;
            }
            if (data.empty() && other.data.empty()) {
                return true;
            }
            folly::IOBufEqualTo eq;
            return eq(*data.front(), *other.data.front());
        }
    };

    struct NoopFrame {
        bool operator==(const NoopFrame&) const {
            return true;
        }
    };

    struct KnobFrame {
        KnobFrame(uint64_t knobSpaceIn, uint64_t idIn, Buf blobIn)
            : knobSpace(knobSpaceIn), id(idIn), blob(std::move(blobIn)) {
            len = blob->length();
        }

        bool operator==(const KnobFrame& rhs) const {
            return knobSpace == rhs.knobSpace && id == rhs.id && len == rhs.len &&
                blob->length() == rhs.blob->length() &&
                memcmp(blob->data(), rhs.blob->data(), blob->length()) == 0;
        }

        KnobFrame& operator=(const KnobFrame& other) {
            knobSpace = other.knobSpace;
            id = other.id;
            if (other.blob) {
                blob = other.blob->clone();
            }
            return *this;
        }

        KnobFrame& operator=(KnobFrame&& other) noexcept {
            knobSpace = other.knobSpace;
            id = other.id;
            if (other.blob) {
                blob = std::move(other.blob);
            }
            return *this;
        }

        KnobFrame(const KnobFrame& other)
            : knobSpace(other.knobSpace), id(other.id), len(other.len), blob(other.blob->clone()) {
        }

        KnobFrame(KnobFrame&& other) noexcept
            : knobSpace(other.knobSpace), id(other.id), len(other.len), blob(std::move(other.blob)) {
        }

        uint64_t knobSpace;
        uint64_t id;
        uint64_t len;
        Buf blob;
    };


    struct WriteAckFrameMetaData {
        // ACK state.
        const WriteAckFrameState& ackState;

        // Delay in sending ack from time that packet was received.
        std::chrono::microseconds ackDelay;
        // The ack delay exponent to use.
        uint8_t ackDelayExponent;

        // Receive timestamps basis
        TimePoint connTime;
    };

    struct WriteAckFrameResult {
        uint64_t bytesWritten;
        WriteAckFrame writeAckFrame;
        // This includes the first ack block
        size_t ackBlocksWritten;
        size_t timestampRangesWritten;
        size_t timestampsWritten;
        WriteAckFrameResult(uint64_t bytesWrittenIn, WriteAckFrame writeAckFrameIn,
            size_t ackBlocksWrittenIn, size_t timestampRangesWrittenIn = 0, size_t timestampsWrittenIn = 0)
            : bytesWritten(bytesWrittenIn), writeAckFrame(std::move(writeAckFrameIn)),
                ackBlocksWritten(ackBlocksWrittenIn), timestampRangesWritten(timestampRangesWrittenIn),
                timestampsWritten(timestampsWrittenIn) {}
    };


    struct QuicSimpleFrame{
        enum class TYPE {
            STOP_SENDING_FRAME,
            PATH_CHALLANGE_FRAME,
            PATH_RESPONSE_FRAME,
            NEW_CONNECTION_ID_FRAME,
            MAX_STREAMS_FRAME,
            RETIRE_CONNECTION_ID_FRAME,
            HANDSHAKE_DONE_FRAME,
            KNOB_FRAME,
            ACK_FREQUENCY_FRAME,
            NEW_TOKEN_FRAME,
        };

        ~QuicSimpleFrame();
        QuicSimpleFrame(QuicSimpleFrame&& other) noexcept;
        QuicSimpleFrame& operator=(QuicSimpleFrame&& other) noexcept;
        QuicSimpleFrame(StopSendingFrame&& in);
        QuicSimpleFrame(PathChallengeFrame&& in);
        QuicSimpleFrame(PathResponseFrame&& in);
        QuicSimpleFrame(NewConnectionIdFrame&& in);
        QuicSimpleFrame(MaxStreamsFrame&& in);
        QuicSimpleFrame(RetireConnectionIdFrame&& in);
        QuicSimpleFrame(HandshakeDoneFrame&& in);
        QuicSimpleFrame(KnobFrame&& in);
        QuicSimpleFrame(AckFrequencyFrame&& in);
        QuicSimpleFrame(NewTokenFrame&& in);

        TYPE type() const;

        StopSendingFrame* asStopSendingFrame();
        PathChallengeFrame* asPathChallengeFrame();
        PathResponseFrame* asPathResponseFrame();
        NewConnectionIdFrame* asNewConnectionIdFrame();
        MaxStreamsFrame* asMaxStreamsFrame();
        RetireConnectionIdFrame* asRetireConnectionIdFrame();
        HandshakeDoneFrame* asHandshakeDoneFrame();
        KnobFrame* asKnobFrame();
        AckFrequencyFrame* asAckFrequencyFrame();
        NewTokenFrame* asNewTokenFrame();
        
    private:
        void destroy() noexcept;

        TYPE _type;
        union{
            StopSendingFrame stopSending;
            PathChallengeFrame pathChallenge;
            PathResponseFrame pathResp;
            NewConnectionIdFrame newConnID;
            MaxStreamsFrame maxStream;
            RetireConnectionIdFrame retireConnID;
            HandshakeDoneFrame handshakeDone;
            KnobFrame knob;
            AckFrequencyFrame ackFrequency;
            NewTokenFrame newToken;
        };
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
            MAX_STREAMS_FRAME,
            DATA_BLOCKED_FRAME,
            STREAM_DATA_BLOCKED_FRAME,
            STREAMS_BLOCKED_FRAME,
            NEW_CONNECTION_ID_FRAME,
            RETIRE_CONNECTION_ID_FRAME,
            PATH_CHALLANGE_FRAME,
            PATH_RESPONSE_FRAME,
            CONNECTION_CLOSE_FRAME,
            HANDSHAKE_DONE_FRAME,
            DATAGRAM_FRAME,
            KNOB_FRAME,
            IMMEDIATE_ACK_FRAME,
            ACK_FREQUENCY_FRAME,
            NOOP_FRAME,
            QUIC_SIMPLE_FRAME,
        };

        ~QuicFrame();
        QuicFrame(QuicFrame&& other) noexcept;
        QuicFrame& operator=(QuicFrame&& other) noexcept;
        QuicFrame(PaddingFrame&& in);
        QuicFrame(PingFrame&& in);
        QuicFrame(ReadAckFrame&& in);
        QuicFrame(WriteAckFrame&& in);
        QuicFrame(RstStreamFrame&& in);
        
        QuicFrame(ReadCryptoFrame&& in);
        QuicFrame(ReadNewTokenFrame&& in);
        QuicFrame(ReadStreamFrame&& in);
        QuicFrame(MaxDataFrame&& in);
        QuicFrame(MaxStreamDataFrame&& in);
        QuicFrame(MaxStreamsFrame&& in);
        QuicFrame(DataBlockedFrame&& in);
        QuicFrame(StreamDataBlockedFrame&& in);
        QuicFrame(StreamsBlockedFrame&& in);
        QuicFrame(NewConnectionIdFrame&& in);
        QuicFrame(RetireConnectionIdFrame&& in);
        QuicFrame(PathChallengeFrame&& in);
        QuicFrame(PathResponseFrame&& in);
        QuicFrame(ConnectionCloseFrame&& in);
        QuicFrame(HandshakeDoneFrame&& in);
        QuicFrame(DatagramFrame&& in);
        QuicFrame(KnobFrame&& in);
        QuicFrame(ImmediateAckFrame&& in);
        QuicFrame(AckFrequencyFrame&& in);
        QuicFrame(NoopFrame&& in);
        QuicFrame(QuicSimpleFrame&& in);

        TYPE type() const;

        PaddingFrame* asPaddingFrame();
        PingFrame* asPingFrame();
        ReadAckFrame* asReadAckFrame();
        WriteAckFrame* asWriteAckFrame();
        RstStreamFrame* asRstStreamFrame();
        StopSendingFrame* asStopSendingFrame();
        ReadCryptoFrame* asReadCryptoFrame();
        ReadNewTokenFrame* asReadNewTokenFrame();
        ReadStreamFrame* asReadStreamFrame();
        MaxDataFrame* asMaxDataFrame();
        MaxStreamDataFrame* asMaxStreamDataFrame();
        MaxStreamsFrame* asMaxStreamsFrame();
        DataBlockedFrame* asDataBlockedFrame();
        StreamDataBlockedFrame* asStreamDataBlockedFrame();
        StreamsBlockedFrame* asStreamsBlockedFrame();
        NewConnectionIdFrame* asNewConnectionIdFrame();
        RetireConnectionIdFrame* asRetireConnectionIdFrame();
        PathChallengeFrame* asPathChallengeFrame();
        PathResponseFrame* asPathResponseFrame();
        ConnectionCloseFrame* asConnectionCloseFrame();
        HandshakeDoneFrame* asHandshakeDoneFrame();
        DatagramFrame* asDatagramFrame();
        KnobFrame* asKnobFrame();
        ImmediateAckFrame* asImmediateAckFrame();
        AckFrequencyFrame* asAckFrequencyFrame();
        NoopFrame* asNoopFrame();
        QuicSimpleFrame* asQuicSimpleFrame();

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
            // TODO NewTokenFrame newToken; //
            ReadStreamFrame readStream;
            MaxDataFrame maxData;
            MaxStreamDataFrame maxStreamData;
            MaxStreamsFrame maxStreams; //
            DataBlockedFrame dataBlocked;
            StreamDataBlockedFrame streamDataBlocked;
            StreamsBlockedFrame streamsBlocked;
            NewConnectionIdFrame newConnID; //
            RetireConnectionIdFrame retireConnID; //
            PathChallengeFrame pathChallenge; //
            PathResponseFrame pathResponse; //
            ConnectionCloseFrame connClose;
            HandshakeDoneFrame handshakeDone; //
            DatagramFrame datagram; //
            KnobFrame knob; //
            ImmediateAckFrame immAck;
            AckFrequencyFrame ackFrequency;
            NoopFrame noop;
            QuicSimpleFrame quicSimple;
        };
    };

    struct QuicWriteFrame{
        enum class TYPE {
            PADDING_FRAME,
            RST_STREAM_FRAME,
            CONNECTION_CLOSE_FRAME,
            MAX_DATA_FRAME,
            MAX_STREAM_DATA_FRAME,
            DATA_BLOCKED_FRAME,
            STREAM_DATA_BLOCKED_FRAME,
            STREAMS_BLOCKED_FRAME,

            WRITE_ACK_FRAME,
            WRITE_STREAM_FRAME,
            WRITE_CRYPTO_FRAME,

            PING_FRAME,
            NOOP_FRAME,
            DATAGRAM_FRAME,
            IMMEDIATE_ACK_FRAME,
            
            QUIC_SIMPLE_FRAME,
        };

        ~QuicWriteFrame();
        QuicWriteFrame(QuicWriteFrame&& other) noexcept;
        QuicWriteFrame& operator=(QuicWriteFrame&& other) noexcept;
        QuicWriteFrame(PaddingFrame&& in);
        QuicWriteFrame(RstStreamFrame&& in);
        QuicWriteFrame(ConnectionCloseFrame&& in);
        QuicWriteFrame(MaxDataFrame&& in);
        QuicWriteFrame(MaxStreamDataFrame&& in);
        QuicWriteFrame(DataBlockedFrame&& in);
        QuicWriteFrame(StreamDataBlockedFrame&& in);
        QuicWriteFrame(StreamsBlockedFrame&& in);

        QuicWriteFrame(WriteAckFrame&& in);
        QuicWriteFrame(WriteStreamFrame&& in);
        QuicWriteFrame(WriteCryptoFrame&& in);

        QuicWriteFrame(PingFrame&& in);
        QuicWriteFrame(NoopFrame&& in);
        QuicWriteFrame(DatagramFrame&& in);
        QuicWriteFrame(ImmediateAckFrame&& in);
        QuicWriteFrame(QuicSimpleFrame&& in);

        TYPE type() const;

        PaddingFrame *asPaddingFrame();
        RstStreamFrame *asRstStreamFrame();
        ConnectionCloseFrame *asConnectionCloseFrame();
        MaxDataFrame *asMaxDataFrame();
        MaxStreamDataFrame *asMaxStreamDataFrame();
        DataBlockedFrame *asDataBlockedFrame();
        StreamDataBlockedFrame *asStreamDataBlockedFrame();
        StreamsBlockedFrame *asStreamsBlockedFrame();
        WriteAckFrame *asWriteAckFrame();
        WriteStreamFrame *asWriteStreamFrame();
        WriteCryptoFrame *asWriteCryptoFrame();
        PingFrame *asPingFrame();
        NoopFrame *asNoopFrame();
        DatagramFrame *asDatagramFrame();
        ImmediateAckFrame *asImmediateAckFrame();
        QuicSimpleFrame *asQuicSimpleFrame();

    private:
        void destroy() noexcept;

        TYPE _type;
        union{
            PaddingFrame padding;
            RstStreamFrame rstStream;
            ConnectionCloseFrame connClose;
            MaxDataFrame maxData;
            MaxStreamDataFrame maxStream;
            DataBlockedFrame dataBlocked;
            StreamDataBlockedFrame streamDataBlocked;
            StreamsBlockedFrame streamBlocked;
            WriteAckFrame writeACK;
            WriteStreamFrame writeStream;
            WriteCryptoFrame writeCrypto;
            PingFrame ping;
            NoopFrame noop;
            DatagramFrame datagram;
            ImmediateAckFrame immAck;
            QuicSimpleFrame quicSimple;
        };
    };

}