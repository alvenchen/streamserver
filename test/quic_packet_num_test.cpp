#include "../src/protocol/quic_packet_num.hpp"
#include <fmt/core.h>

using namespace quic;

int main(int ac, char** av) {

    /*
        For example, if an endpoint has received an acknowledgment for packet 0xabe8b3 
        and is sending a packet with a number of 0xac5c02, 
        there are 29,519 (0x734f) outstanding packet numbers. 
        In order to represent at least twice this range (59,038 packets, or 0xe69e), 
        16 bits are required
    */

    PacketNum packetN = 0xac5c02;
    PacketNum ackN = 0xabe8b3;
    PacketNumEncodingResult packetNumEncoded = encodePacketNumber(packetN, ackN);

    fmt::print("The answer is 0x{:x}, len:{}\n", packetNumEncoded.result, packetNumEncoded.length);
    
    return 0;
}