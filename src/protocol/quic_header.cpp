#include "quic_header.hpp"

namespace quic{

HeaderForm getHeaderForm(uint8_t headerValue){
    if (headerValue & kHeaderFormMask) {
        return HeaderForm::Long;
    }
    return HeaderForm::Short;
}


LongHeader::LongHeader(Types type, LongHeaderInvariant invariant, std::string token)
    : _longHeaderType(type), _invariant(std::move(invariant)), _token(std::move(token)), _packetSequenceNum(0) {

}

LongHeader::LongHeader(Types type, const ConnectionId& srcConnId, const ConnectionId& dstConnId, 
    PacketNum packetNum, QuicVersion version, std::string token)
        : _longHeaderType(type), _invariant(LongHeaderInvariant(version, srcConnId, dstConnId)),
        _token(std::move(token)), _packetSequenceNum(0) {
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


/*short header*/
ShortHeaderInvariant::ShortHeaderInvariant(ConnectionId dcid)
    : destinationConnId(std::move(dcid)){
}

ShortHeader::ShortHeader(ProtectionType protectionType, ConnectionId connId, PacketNum packetNum)
    : _protectionType(protectionType), _connectionId(std::move(connId)), _packetSequenceNum(0) {
    if (_protectionType != ProtectionType::KeyPhaseZero &&
        _protectionType != ProtectionType::KeyPhaseOne) {
        throw std::logic_error("bad short header protection type");
    }
    setPacketNumber(packetNum);
}

ShortHeader::ShortHeader(ProtectionType protectionType, ConnectionId connId)
    : _protectionType(protectionType), _connectionId(std::move(connId)), _packetSequenceNum(0) {
    if (_protectionType != ProtectionType::KeyPhaseZero &&
        _protectionType != ProtectionType::KeyPhaseOne) {

        throw std::logic_error("bad short header protection type");
    }
}

ProtectionType ShortHeader::getProtectionType() const {
    return _protectionType;
}

const ConnectionId& ShortHeader::getConnectionId() const {
    return _connectionId;
}

void ShortHeader::setPacketNumber(PacketNum packetNum) {
    _packetSequenceNum = packetNum;
}


PacketHeader::~PacketHeader() {
    destroyHeader();
}

PacketHeader::PacketHeader(ShortHeader&& shortHeaderIn)
    : _headerForm(HeaderForm::Short) {
    new (&shortHeader) ShortHeader(std::move(shortHeaderIn));
}

PacketHeader::PacketHeader(LongHeader&& longHeaderIn)
    : _headerForm(HeaderForm::Long) {
    new (&longHeader) LongHeader(std::move(longHeaderIn));
}

PacketHeader::PacketHeader(const PacketHeader& other)
    : _headerForm(other._headerForm) {
    switch (other._headerForm) {
        case HeaderForm::Long:
            new (&longHeader) LongHeader(other.longHeader);
            break;
        case HeaderForm::Short:
            new (&shortHeader) ShortHeader(other.shortHeader);
            break;
    }
}

PacketHeader::PacketHeader(PacketHeader&& other) noexcept
    : _headerForm(other._headerForm) {
    switch (other._headerForm) {
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
    switch (other._headerForm) {
        case HeaderForm::Long:
            new (&longHeader) LongHeader(std::move(other.longHeader));
            break;
        case HeaderForm::Short:
            new (&shortHeader) ShortHeader(std::move(other.shortHeader));
            break;
    }
    _headerForm = other._headerForm;
    return *this;
}

PacketHeader& PacketHeader::operator=(const PacketHeader& other) {
    destroyHeader();
    switch (other._headerForm) {
        case HeaderForm::Long:
            new (&longHeader) LongHeader(other.longHeader);
            break;
        case HeaderForm::Short:
            new (&shortHeader) ShortHeader(other.shortHeader);
            break;
    }
    _headerForm = other._headerForm;
    return *this;
}

void PacketHeader::destroyHeader() {
    switch (_headerForm) {
        case HeaderForm::Long:
            longHeader.~LongHeader();
            break;
        case HeaderForm::Short:
            shortHeader.~ShortHeader();
            break;
    }
}

LongHeader* PacketHeader::asLong() {
    switch (_headerForm) {
        case HeaderForm::Long:
            return &longHeader;
        case HeaderForm::Short:
            return nullptr;
        default:
            folly::assume_unreachable();
    }
}

ShortHeader* PacketHeader::asShort() {
    switch (_headerForm) {
        case HeaderForm::Long:
            return nullptr;
        case HeaderForm::Short:
            return &shortHeader;
        default:
            folly::assume_unreachable();
    }
}

const LongHeader* PacketHeader::asLong() const {
    switch (_headerForm) {
        case HeaderForm::Long:
            return &longHeader;
        case HeaderForm::Short:
            return nullptr;
        default:
            folly::assume_unreachable();
    }
}

const ShortHeader* PacketHeader::asShort() const {
    switch (_headerForm) {
        case HeaderForm::Long:
            return nullptr;
        case HeaderForm::Short:
            return &shortHeader;
        default:
            folly::assume_unreachable();
    }
}

HeaderForm PacketHeader::getHeaderForm() const {
    return _headerForm;
}

ProtectionType PacketHeader::getProtectionType() const {
    switch (_headerForm) {
        case HeaderForm::Long:
            return longHeader.getProtectionType();
        case HeaderForm::Short:
            return shortHeader.getProtectionType();
        default:
            folly::assume_unreachable();
    }
}


/*
    function
*/
LongHeader::Types parseLongHeaderType(uint8_t initialByte) {
    return static_cast<LongHeader::Types>((initialByte & LongHeader::kPacketTypeMask) >> LongHeader::kTypeShift);
}


ProtectionType longHeaderTypeToProtectionType(LongHeader::Types longHeaderType) {
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

PacketNumberSpace protectionTypeToPacketNumberSpace(ProtectionType protectionType) {
    switch (protectionType) {
        case ProtectionType::Initial:
            return PacketNumberSpace::Initial;
        case ProtectionType::Handshake:
            return PacketNumberSpace::Handshake;
        case ProtectionType::ZeroRtt:
        case ProtectionType::KeyPhaseZero:
        case ProtectionType::KeyPhaseOne:
            return PacketNumberSpace::AppData;
    }
    folly::assume_unreachable();
}

}