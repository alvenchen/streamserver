#pragma once

#include "../common/common.hpp"
#include "../folly/io/Cursor.h"

namespace quic {
constexpr size_t kMaxConnectionIdSize = 20;

constexpr uint8_t kStatelessResetTokenLength = 16;
using StatelessResetToken = std::array<uint8_t, kStatelessResetTokenLength>;

struct ConnectionId {
    explicit ConnectionId(const std::vector<uint8_t>& connidIn);
    explicit ConnectionId(folly::io::Cursor& cursor, size_t len);

    bool operator==(const ConnectionId& other) const;
    bool operator!=(const ConnectionId& other) const;


    uint8_t* data();
    uint8_t size() const;    
    //std::string Hex() const;

    /**
     * Create an connection without any checks for tests.
     */
    static ConnectionId createWithoutChecks(const std::vector<uint8_t>& connidIn);

    /**
     * Create a random ConnectionId with the given length.
     */
    static ConnectionId createRandom(size_t len);

private:
    ConnectionId() = default;

    std::array<uint8_t, kMaxConnectionIdSize> _connID;
    uint8_t _connIDLen;
};

}