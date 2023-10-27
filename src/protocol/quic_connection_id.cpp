#include "quic_connection_id.hpp"
#include "../folly/Random.h"
#include <string>

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


uint8_t* ConnectionId::data() const{
    return _connID.data();
}

uint8_t ConnectionId::size() const{
    return _connIDLen;
}

std::string ConnectionId::hex() const{
    static char hexValues[] = "0123456789abcdef";
    std::string output;
    
    output.resize(2 * _connIDLen);
    size_t j = 0;
    for (size_t i = 0; i < _connIDLen; ++i) {
        int ch = _connID[i];
        output[j++] = hexValues[(ch >> 4) & 0xf];
        output[j++] = hexValues[ch & 0xf];
    }
    return output;
}

ConnectionId ConnectionId::createWithoutChecks(const std::vector<uint8_t>& connidIn) {
    ConnectionId connid;
    connid._connIDLen = connidIn.size();
    if (connid._connIDLen != 0) {
        memcpy(connid._connID.data(), connidIn.data(), connid._connIDLen);
    }
    return connid;
}

ConnectionId ConnectionId::createRandom(size_t len) {
    ConnectionId connid;
    if (len > kMaxConnectionIdSize) {
        throw std::runtime_error("ConnectionId invalid size");
    }
    connid._connIDLen = len;
    folly::Random::secureRandom(connid._connID.data(), connid._connIDLen);
    return connid;
}


}