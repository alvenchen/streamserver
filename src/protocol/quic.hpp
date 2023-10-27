#include <cstdint>
#include <cstddef>
#include <vector>
#include "quic_packet_num.hpp"
#include "../common/IntervalSet.h"

#pragma once

namespace quic{
    using StreamId = uint64_t;
    using StreamGroupId = uint64_t;

    template <class T, size_t N>
    using SmallVec = std::vector<T>;

    constexpr uint8_t kHeaderFormMask = 0x80;
    constexpr uint32_t kMaxPacketNumEncodingSize = 4;
    constexpr uint32_t kNumInitialAckBlocksPerFrame = 32;

    template <class T>
    using IntervalSetVec = SmallVec<T, kNumInitialAckBlocksPerFrame>;
    using AckBlocks = IntervalSet<PacketNum, 1, IntervalSetVec>;
}

