#include "quic_packet_num.hpp"

namespace quic{

/*
    https://datatracker.ietf.org/doc/html/rfc9000#sample-packet-number-encoding
*/
PacketNumEncodingResult EncodePacketNumber(PacketNum packetNum, PacketNum largestAckedPacketNum){
    PacketNum twiceDistance = (packetNum - largestAckedPacketNum) * 2;
    // The number of bits we need to mask all set bits in twiceDistance.
    // This is 1 + floor(log2(x)).
    uint32_t lengthInBits = common::FindLastSet(twiceDistance);
    // Round up to bytes
    uint32_t lengthInBytes = lengthInBits == 0 ? 1 : (lengthInBits + 7) >> 3;
    if (lengthInBytes > 4) {
        throw std::runtime_error("Impossible to encode PacketNum");
    }
    // We need a mask that's all 1 for lengthInBytes bytes. Left shift a 1 by that
    // many bits and then -1 will give us that. Or if lengthInBytes is 8, then ~0
    // will just do it.
    uint64_t mask = (1ULL << lengthInBytes * 8) - 1;
    return PacketNumEncodingResult(packetNum & mask, lengthInBytes);
}

/*
    https://datatracker.ietf.org/doc/html/rfc9000#name-sample-packet-number-decodi
*/
PacketNum DecodePacketNumber(uint64_t encodedPacketNum, uint32_t packetNumBytes, PacketNum expectedNextPacketNum){
    size_t packetNumBits = 8 * packetNumBytes;
    PacketNum packetNumWin = 1ULL << packetNumBits;
    PacketNum packetNumHalfWin = packetNumWin >> 1;
    PacketNum mask = packetNumWin - 1;
    PacketNum candidate = (expectedNextPacketNum & ~mask) | encodedPacketNum;
    if (expectedNextPacketNum > packetNumHalfWin &&
        candidate <= expectedNextPacketNum - packetNumHalfWin &&
        candidate < (1ULL << 62) - packetNumWin) {
        return candidate + packetNumWin;
    }
    if (candidate > expectedNextPacketNum + packetNumHalfWin &&
        candidate >= packetNumWin) {
        return candidate - packetNumWin;
    }
    return candidate;
}


PacketNumEncodingResult::PacketNumEncodingResult(PacketNum resultIn, uint32_t lengthIn)
    :result(resultIn), length(lengthIn){
}


}