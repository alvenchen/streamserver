#pragma once

#include <cstdint>
#include "../common.hpp"

namespace quic{
    using PacketNum = uint64_t;

    struct PacketNumEncodingResult {
        PacketNum result;
        // This is packet number length in bytes
        uint32_t length;

        PacketNumEncodingResult(PacketNum resultIn, uint32_t lengthIn);
    };

    PacketNumEncodingResult EncodePacketNumber(PacketNum packetNum, PacketNum largestAckedPacketNum);
    PacketNum DecodePacketNumber(uint64_t encodedPacketNum, uint32_t packetNumBytes, PacketNum expectedNextPacketNum);

}