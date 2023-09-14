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




PacketHeader::PacketHeader(ShortHeader&& shortHeaderIn)
    : headerForm_(HeaderForm::Short) {
    new (&shortHeader) ShortHeader(std::move(shortHeaderIn));
}

PacketHeader::PacketHeader(LongHeader&& longHeaderIn)
    : headerForm_(HeaderForm::Long) {
    new (&longHeader) LongHeader(std::move(longHeaderIn));
}

PacketHeader::PacketHeader(const PacketHeader& other)
    : headerForm_(other.headerForm_) {
    switch (other.headerForm_) {
        case HeaderForm::Long:
            new (&longHeader) LongHeader(other.longHeader);
            break;
        case HeaderForm::Short:
            new (&shortHeader) ShortHeader(other.shortHeader);
            break;
    }
}

PacketHeader::PacketHeader(PacketHeader&& other) noexcept
    : headerForm_(other.headerForm_) {
    switch (other.headerForm_) {
        case HeaderForm::Long:
            new (&longHeader) LongHeader(std::move(other.longHeader));
            break;
        case HeaderForm::Short:
            new (&shortHeader) ShortHeader(std::move(other.shortHeader));
            break;
    }
}

PacketHeader& PacketHeader::operator=(PacketHeader&& other) noexcept {
    destroyHeader();
    switch (other.headerForm_) {
        case HeaderForm::Long:
            new (&longHeader) LongHeader(std::move(other.longHeader));
            break;
        case HeaderForm::Short:
            new (&shortHeader) ShortHeader(std::move(other.shortHeader));
            break;
    }
    headerForm_ = other.headerForm_;
    return *this;
}

PacketHeader& PacketHeader::operator=(const PacketHeader& other) {
    destroyHeader();
    switch (other.headerForm_) {
        case HeaderForm::Long:
            new (&longHeader) LongHeader(other.longHeader);
            break;
        case HeaderForm::Short:
            new (&shortHeader) ShortHeader(other.shortHeader);
            break;
    }
    headerForm_ = other.headerForm_;
    return *this;
}

PacketHeader::~PacketHeader() {
    destroyHeader();
}

void PacketHeader::destroyHeader() {
    switch (headerForm_) {
        case HeaderForm::Long:
            longHeader.~LongHeader();
            break;
        case HeaderForm::Short:
            shortHeader.~ShortHeader();
            break;
    }
}

LongHeader* PacketHeader::asLong() {
    switch (headerForm_) {
        case HeaderForm::Long:
            return &longHeader;
        case HeaderForm::Short:
            return nullptr;
        default:
            folly::assume_unreachable();
    }
}

ShortHeader* PacketHeader::asShort() {
    switch (headerForm_) {
        case HeaderForm::Long:
            return nullptr;
        case HeaderForm::Short:
            return &shortHeader;
        default:
            folly::assume_unreachable();
    }
}

const LongHeader* PacketHeader::asLong() const {
    switch (headerForm_) {
        case HeaderForm::Long:
            return &longHeader;
        case HeaderForm::Short:
            return nullptr;
        default:
            folly::assume_unreachable();
    }
}

const ShortHeader* PacketHeader::asShort() const {
    switch (headerForm_) {
        case HeaderForm::Long:
            return nullptr;
        case HeaderForm::Short:
            return &shortHeader;
        default:
            folly::assume_unreachable();
    }
}

HeaderForm PacketHeader::getHeaderForm() const {
    return headerForm_;
}

ProtectionType PacketHeader::getProtectionType() const {
    switch (headerForm_) {
        case HeaderForm::Long:
            return longHeader.getProtectionType();
        case HeaderForm::Short:
            return shortHeader.getProtectionType();
        default:
            folly::assume_unreachable();
    }
}





}