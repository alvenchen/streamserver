#include "quic_type.hpp"

namespace quic
{
    
bool StreamTypeField::hasDataLength() const {
    return field_ & kDataLengthBit;
}

bool StreamTypeField::hasFin() const {
    return field_ & kFinBit;
}

bool StreamTypeField::hasOffset() const {
    return field_ & kOffsetBit;
}

uint8_t StreamTypeField::fieldValue() const {
    return field_;
}

StreamTypeField::Builder& StreamTypeField::Builder::switchToStreamGroups() {
    field_ = static_cast<uint8_t>(FrameType::GROUP_STREAM);
    return *this;
}

StreamTypeField::Builder& StreamTypeField::Builder::setFin() {
    field_ |= StreamTypeField::kFinBit;
    return *this;
}

StreamTypeField::Builder& StreamTypeField::Builder::setOffset() {
    field_ |= StreamTypeField::kOffsetBit;
    return *this;
}

StreamTypeField::Builder& StreamTypeField::Builder::setLength() {
    field_ |= StreamTypeField::kDataLengthBit;
    return *this;
}

StreamTypeField StreamTypeField::Builder::build() {
    return StreamTypeField(field_);
}


} // namespace quic
