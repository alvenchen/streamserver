#pragma once

#include "../common.hpp"

namespace quic {
constexpr size_t kMaxConnectionIdSize = 20;

struct ConnectionId {
    explicit ConnectionId(const std::vector<uint8_t>& connidIn);
    bool operator==(const ConnectionId& other) const;
    bool operator!=(const ConnectionId& other) const;


    uint8_t* Data();
    uint8_t Size() const;    
    std::string Hex() const;

    /**
     * Create an connection without any checks for tests.
     */
    static ConnectionId CreateWithoutChecks(const std::vector<uint8_t>& connidIn);

    /**
     * Create a random ConnectionId with the given length.
     */
    static ConnectionId CreateRandom(size_t len);

private:
    ConnectionId() = default;

    std::array<uint8_t, kMaxConnectionIdSize> _connID;
    uint8_t _connIDLen;
};

}