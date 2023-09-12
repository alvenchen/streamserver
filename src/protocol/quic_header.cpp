#include "quic_header.hpp"

namespace quic{

HeaderForm getHeaderForm(uint8_t headerValue){
    if (headerValue & kHeaderFormMask) {
        return HeaderForm::Long;
    }
    return HeaderForm::Short;
}


LongHeader::LongHeader(Types type, LongHeaderInvariant invariant, std::string token)
    : _longHeaderType(type), _invariant(std::move(invariant)), _token(std::move(token)) {

}

LongHeader::LongHeader(Types type, const ConnectionId& srcConnId, const ConnectionId& dstConnId, 
    PacketNum packetNum, QuicVersion version, std::string token)
        : _longHeaderType(type), _invariant(LongHeaderInvariant(version, srcConnId, dstConnId)),
        _token(std::move(token)) {
    setPacketNumber(packetNum);
}


LongHeader::Types LongHeader::getHeaderType() const noexcept {
    return _longHeaderType;
}

const ConnectionId& LongHeader::getSourceConnId() const {
    return _invariant.srcConnId;
}

const ConnectionId& LongHeader::getDestinationConnId() const {
    return _invariant.dstConnId;
}

QuicVersion LongHeader::getVersion() const {
    return _invariant.version;
}

bool LongHeader::hasToken() const {
    return !_token.empty();
}

const std::string& LongHeader::getToken() const {
    return _token;
}

void LongHeader::setPacketNumber(PacketNum packetNum) {
    _packetSequenceNum = packetNum;
}

ProtectionType LongHeader::getProtectionType() const {
    return longHeaderTypeToProtectionType(getHeaderType());
}

ProtectionType longHeaderTypeToProtectionType(
    LongHeader::Types longHeaderType) {
    switch (longHeaderType) {
        case LongHeader::Types::Initial:
        case LongHeader::Types::Retry:
            return ProtectionType::Initial;
        case LongHeader::Types::Handshake:
            return ProtectionType::Handshake;
        case LongHeader::Types::ZeroRtt:
            return ProtectionType::ZeroRtt;
    }
}


/*short header*/
ShortHeaderInvariant::ShortHeaderInvariant(ConnectionId dcid)
    : destinationConnId(std::move(dcid)){
}

ShortHeader::ShortHeader(ProtectionType protectionType, ConnectionId connId, PacketNum packetNum)
    : protectionType_(protectionType), connectionId_(std::move(connId)) {
    if (protectionType_ != ProtectionType::KeyPhaseZero &&
        protectionType_ != ProtectionType::KeyPhaseOne) {
        throw std::logic_error("bad short header protection type");
    }
    setPacketNumber(packetNum);
}

ShortHeader::ShortHeader(ProtectionType protectionType, ConnectionId connId)
    : protectionType_(protectionType), connectionId_(std::move(connId)) {
    if (protectionType_ != ProtectionType::KeyPhaseZero &&
        protectionType_ != ProtectionType::KeyPhaseOne) {

        throw std::logic_error("bad short header protection type");
    }
}

ProtectionType ShortHeader::getProtectionType() const {
    return protectionType_;
}

const ConnectionId& ShortHeader::getConnectionId() const {
    return connectionId_;
}

void ShortHeader::setPacketNumber(PacketNum packetNum) {
    packetSequenceNum_ = packetNum;
}


}