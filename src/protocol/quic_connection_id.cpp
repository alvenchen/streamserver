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

ConnectionId::ConnectionId(folly::io::Cursor& cursor, size_t len) {
    // Zero is special case for connids.
    if (len == 0) {
        _connIDLen = 0;
        return;
    }
    if (len > kMaxConnectionIdSize) {
        // We can't throw a transport error here because of the dependency. This is
        // sad because this will cause an internal error downstream.
        throw std::runtime_error("ConnectionId invalid size");
    }
    _connIDLen = len;
    cursor.pull(_connID.data(), len);
}

bool ConnectionId::operator==(const ConnectionId& other) const {
    return _connIDLen == other._connIDLen &&
        memcmp(_connID.data(), other._connID.data(), _connIDLen) == 0;
}

bool ConnectionId::operator!=(const ConnectionId& other) const {
    return !operator==(other);
}


uint8_t* ConnectionId::data(){
    return _connID.data();
}

uint8_t ConnectionId::size() const{
    return _connIDLen;
}


}