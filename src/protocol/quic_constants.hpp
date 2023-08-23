#pragma once

#include "common.hpp"

namespace quic {

enum class QuicVersion : uint32_t {
  VERSION_NEGOTIATION = 0x00000000,
  
  QUIC_DRAFT = 0xff00001d, // Draft-29
  QUIC_V1 = 0x00000001,
  QUIC_V1_ALIAS = 0xfaceb003,
  QUIC_TLS = 0x101,
  
};

}