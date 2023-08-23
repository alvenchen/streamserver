#include "quic_connection_id.hpp"

namespace quic {

ConnectionId::ConnectionId(const std::vector<uint8_t>& connidIn){
    if (connidIn.size() > kMaxConnectionIdSize) {
        throw std::runtime_error("ConnectionId invalid size");
    }
    _connIDLen = connidIn.size();
    if (_connIDLen != 0) {
        memcpy(_connID.data(), connidIn.data(), _connIDLen);
    }
}

uint8_t* ConnectionId::Data(){
    return _connID.data();
}

uint8_t ConnectionId::Size() const{
    return _connIDLen;
}

bool ConnectionId::operator==(const ConnectionId& other) const {
  return _connIDLen == other._connIDLen &&
      memcmp(_connID.data(), other._connID.data(), _connIDLen) == 0;
}

bool ConnectionId::operator!=(const ConnectionId& other) const {
  return !operator==(other);
}

}