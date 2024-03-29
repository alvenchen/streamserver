#include <cstdint>
#include <cstddef>
#include <vector>
#include "quic_packet_num.hpp"
#include "common/IntervalSet.h"

#pragma once

namespace quic{
    using StreamId = uint64_t;
    using StreamGroupId = uint64_t;

    template <class T, std::size_t N, class... Policy>
    using SmallVec = std::vector<T>;

    constexpr uint8_t kHeaderFormMask = 0x80;
    constexpr uint32_t kMaxPacketNumEncodingSize = 4;
    constexpr uint32_t kNumInitialAckBlocksPerFrame = 32;

    template <class T>
    using IntervalSetVec = SmallVec<T, kNumInitialAckBlocksPerFrame>;
    using AckBlocks = IntervalSet<PacketNum, 1, IntervalSetVec>;

    template <typename T>
    inline
    T GetTypedBuf(char* buf, size_t offset) {
        return *(reinterpret_cast<T*>(buf + offset));
    }

    template <typename T>
    inline
    T GetTypedBuf(const char* buf, size_t offset) {
        return *(reinterpret_cast<T*>(buf + offset));
    }
}

