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



std::string toString(FrameType frame){
    switch (frame) {
    case FrameType::PADDING:
        return "PADDING";
    case FrameType::PING:
        return "PING";
    case FrameType::ACK:
        return "ACK";
    case FrameType::ACK_ECN:
        return "ACK_ECN";
    case FrameType::RST_STREAM:
        return "RST_STREAM";
    case FrameType::STOP_SENDING:
        return "STOP_SENDING";
    case FrameType::CRYPTO_FRAME:
        return "CRYPTO_FRAME";
    case FrameType::NEW_TOKEN:
        return "NEW_TOKEN";
    case FrameType::STREAM:
    case FrameType::STREAM_FIN:
    case FrameType::STREAM_LEN:
    case FrameType::STREAM_LEN_FIN:
    case FrameType::STREAM_OFF:
    case FrameType::STREAM_OFF_FIN:
    case FrameType::STREAM_OFF_LEN:
    case FrameType::STREAM_OFF_LEN_FIN:
        return "STREAM";
    case FrameType::MAX_DATA:
        return "MAX_DATA";
    case FrameType::MAX_STREAM_DATA:
        return "MAX_STREAM_DATA";
    case FrameType::MAX_STREAMS_BIDI:
        return "MAX_STREAMS_BIDI";
    case FrameType::MAX_STREAMS_UNI:
        return "MAX_STREAMS_UNI";
    case FrameType::DATA_BLOCKED:
        return "DATA_BLOCKED";
    case FrameType::STREAM_DATA_BLOCKED:
        return "STREAM_DATA_BLOCKED";
    case FrameType::STREAMS_BLOCKED_BIDI:
        return "STREAMS_BLOCKED_BIDI";
    case FrameType::STREAMS_BLOCKED_UNI:
        return "STREAMS_BLOCKED_UNI";
    case FrameType::NEW_CONNECTION_ID:
        return "NEW_CONNECTION_ID";
    case FrameType::RETIRE_CONNECTION_ID:
        return "RETIRE_CONNECTION_ID";
    case FrameType::PATH_CHALLENGE:
        return "PATH_CHALLENGE";
    case FrameType::PATH_RESPONSE:
        return "PATH_RESPONSE";
    case FrameType::CONNECTION_CLOSE:
        return "CONNECTION_CLOSE";
    case FrameType::CONNECTION_CLOSE_APP_ERR:
        return "APPLICATION_CLOSE";
    case FrameType::HANDSHAKE_DONE:
        return "HANDSHAKE_DONE";
    case FrameType::DATAGRAM:
    case FrameType::DATAGRAM_LEN:
        return "DATAGRAM";
    case FrameType::KNOB:
        return "KNOB";
    case FrameType::ACK_FREQUENCY:
        return "ACK_FREQUENCY";
    case FrameType::IMMEDIATE_ACK:
        return "IMMEDIATE_ACK";
    case FrameType::GROUP_STREAM:
    case FrameType::GROUP_STREAM_FIN:
    case FrameType::GROUP_STREAM_LEN:
    case FrameType::GROUP_STREAM_LEN_FIN:
    case FrameType::GROUP_STREAM_OFF:
    case FrameType::GROUP_STREAM_OFF_FIN:
    case FrameType::GROUP_STREAM_OFF_LEN:
    case FrameType::GROUP_STREAM_OFF_LEN_FIN:
        return "GROUP_STREAM";
    case FrameType::ACK_RECEIVE_TIMESTAMPS:
        return "ACK_RECEIVE_TIMESTAMPS";
    }
    //LOG(WARNING) << "toString has unhandled frame type";
    return "UNKNOWN";
}

std::string toString(TokenType type){
    switch (type) {
        case TokenType::RetryToken:
            return "RetryToken";
        case TokenType::NewToken:
            return "NewToken";
    }
    //LOG(WARNING) << "toString has unhandled token type";
    return "UNKNOWN";
}

std::string toString(QuicVersion version){
    switch (version) {
        case QuicVersion::VERSION_NEGOTIATION:
        return "VERSION_NEGOTIATION";
        case QuicVersion::MVFST:
        return "MVFST";
        case QuicVersion::QUIC_V1:
        return "QUIC_V1";
        case QuicVersion::QUIC_V1_ALIAS:
        return "QUIC_V1_ALIAS";
        case QuicVersion::QUIC_DRAFT:
        return "QUIC_DRAFT";
        case QuicVersion::MVFST_EXPERIMENTAL:
        return "MVFST_EXPERIMENTAL";
        case QuicVersion::MVFST_ALIAS:
        return "MVFST_ALIAS";
        case QuicVersion::MVFST_INVALID:
        return "MVFST_INVALID";
        case QuicVersion::MVFST_EXPERIMENTAL2:
        return "MVFST_EXPERIMENTAL2";
        case QuicVersion::MVFST_EXPERIMENTAL3:
        return "MVFST_EXPERIMENTAL3";
    }
    //LOG(WARNING) << "toString has unhandled version type";
    return "UNKNOWN";
}


std::string toString(PacketNumberSpace pnSpace) {
    switch (pnSpace) {
    case PacketNumberSpace::Initial:
        return "InitialSpace";
    case PacketNumberSpace::Handshake:
        return "HandshakeSpace";
    case PacketNumberSpace::AppData:
        return "AppDataSpace";
    }
    //CHECK(false) << "Unknown packet number space";
    folly::assume_unreachable();
}

std::string toString(ProtectionType protectionType){
    switch (protectionType) {
        case ProtectionType::Initial:
        return "Initial";
        case ProtectionType::Handshake:
        return "Handshake";
        case ProtectionType::ZeroRtt:
        return "ZeroRtt";
        case ProtectionType::KeyPhaseZero:
        return "KeyPhaseZero";
        case ProtectionType::KeyPhaseOne:
        return "KeyPhaseOne";
    }
    //CHECK(false) << "Unknown protection type";
    folly::assume_unreachable();
}


} // namespace quic
