
#pragma once

#include <optional>
#include "../common/common.hpp"
#include "quic_constants.hpp"
#include "quic_packet_num.hpp"
#include "quic.hpp"
#include "quic_frame.hpp"
#include "quic_connection_id.hpp"

namespace quic{
    
struct StreamTypeField {
public:
    explicit StreamTypeField(uint8_t field) : field_(field) {}
    bool hasFin() const;
    bool hasDataLength() const;
    bool hasOffset() const;
    uint8_t fieldValue() const;

    struct Builder {
    public:
        Builder() : field_(static_cast<uint8_t>(FrameType::STREAM)) {}
        Builder& switchToStreamGroups();
        Builder& setFin();
        Builder& setOffset();
        Builder& setLength();

        StreamTypeField build();

    private:
        uint8_t field_;
    };

private:
    // Stream Frame specific:
    static constexpr uint8_t kFinBit = 0x01;
    static constexpr uint8_t kDataLengthBit = 0x02;
    static constexpr uint8_t kOffsetBit = 0x04;

    uint8_t field_;
};

struct StatelessReset {
    StatelessResetToken token;

    explicit StatelessReset(StatelessResetToken tokenIn)
        : token(std::move(tokenIn)) {}
};

} // namespace quic
