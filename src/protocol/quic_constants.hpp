#pragma once

#include "../common/common.hpp"

#include <sys/types.h>
#include <chrono>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>


namespace quic {

    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using DurationRep = std::chrono::microseconds::rep;
    using namespace std::chrono_literals;


    enum class QuicVersion : uint32_t {
        VERSION_NEGOTIATION = 0x00000000,

        QUIC_DRAFT = 0xff00001d, // Draft-29
        QUIC_V1 = 0x00000001,
        QUIC_V1_ALIAS = 0xfaceb003,
        QUIC_TLS = 0x101,

    };

    enum class FrameType : uint64_t {
        PADDING = 0x00,
        PING = 0x01,
        ACK = 0x02,
        ACK_ECN = 0x03,
        RST_STREAM = 0x04,
        STOP_SENDING = 0x05,
        CRYPTO_FRAME = 0x06, // librtmp has a #define CRYPTO
        NEW_TOKEN = 0x07,
        // STREAM frame can have values from 0x08 to 0x0f which indicate which fields
        // are present in the frame.
        STREAM = 0x08,
        STREAM_FIN = 0x09,
        STREAM_LEN = 0x0a,
        STREAM_LEN_FIN = 0x0b,
        STREAM_OFF = 0x0c,
        STREAM_OFF_FIN = 0x0d,
        STREAM_OFF_LEN = 0x0e,
        STREAM_OFF_LEN_FIN = 0x0f,
        MAX_DATA = 0x10,
        MAX_STREAM_DATA = 0x11,
        MAX_STREAMS_BIDI = 0x12,
        MAX_STREAMS_UNI = 0x13,
        DATA_BLOCKED = 0x14,
        STREAM_DATA_BLOCKED = 0x15,
        STREAMS_BLOCKED_BIDI = 0x16,
        STREAMS_BLOCKED_UNI = 0x17,
        NEW_CONNECTION_ID = 0x18,
        RETIRE_CONNECTION_ID = 0x19,
        PATH_CHALLENGE = 0x1A,
        PATH_RESPONSE = 0x1B,
        CONNECTION_CLOSE = 0x1C,
        // CONNECTION_CLOSE_APP_ERR frametype is use to indicate application errors
        CONNECTION_CLOSE_APP_ERR = 0x1D,
        HANDSHAKE_DONE = 0x1E,
        DATAGRAM = 0x30,
        DATAGRAM_LEN = 0x31,
        KNOB = 0x1550,
        IMMEDIATE_ACK = 0xAC,
        ACK_FREQUENCY = 0xAF,
        // Stream groups.
        GROUP_STREAM = 0x32,
        GROUP_STREAM_FIN = 0x33,
        GROUP_STREAM_LEN = 0x34,
        GROUP_STREAM_LEN_FIN = 0x35,
        GROUP_STREAM_OFF = 0x36,
        GROUP_STREAM_OFF_FIN = 0x37,
        GROUP_STREAM_OFF_LEN = 0x38,
        GROUP_STREAM_OFF_LEN_FIN = 0x39,
        ACK_RECEIVE_TIMESTAMPS = 0xB0
    };

}