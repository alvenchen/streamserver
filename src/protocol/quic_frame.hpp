#pragma once

#include <optional>
#include "../common/common.hpp"
#include "quic_constants.hpp"
#include "quic_packet_num.hpp"
#include <common/IntervalSet.h>

namespace quic{
    template <class T>
    using SmallVec = std::vector<T>;

    //generic type
    struct QuicFrame{
        enum class TYPE {
            PADDING_FRAME,
            PING_FRAME,
            RST_STREAM_FRAME,

        };

        ~QuicFrame();
        QuicFrame(QuicFrame&& other) noexcept;
        QuicFrame& operator=(QuicFrame&& other) noexcept;
        QuicFrame(PaddingFrame&& in);
        QuicFrame(PingFrame&& in);

        TYPE type();

        PaddingFrame* paddingFrame();
        PingFrame* pingFrame();

    private:
        void destroy();

        TYPE _type;
        union{
            PaddingFrame padding;
            PingFrame ping;

        };
    };

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



}