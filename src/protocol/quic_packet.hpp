#pragma once


#include "../common/common.hpp"
#include "quic_connection_id.hpp"
#include "quic_packet_num.hpp"
#include "quic_constants.hpp"
#include "../folly/io/Cursor.h"
#include "quic_header.hpp"
#include "quic_frame.hpp"

namespace quic {

struct RegularPacket {
    PacketHeader header;

    explicit RegularPacket(PacketHeader&& headerIn)
      : header(std::move(headerIn)) {}
};

/**
 * A representation of a regular packet that is read from the network.
 * This could be either Cleartext or Encrypted packets in long or short form.
 * Cleartext packets include Client Initial, Client Cleartext, Non-Final Server
 * Cleartext packet or Final Server Cleartext packet. Encrypted packets
 * include 0-RTT, 1-RTT Phase 0 and 1-RTT Phase 1 packets.
 */
struct RegularQuicPacket : public RegularPacket {
    using Vec = SmallVec<QuicFrame, 4>;
    Vec frames;

    explicit RegularQuicPacket(PacketHeader&& headerIn) : RegularPacket(std::move(headerIn)) {
    }
};



}