/*
 
*/

#pragma once


#include "../common.hpp"
#include "quic_connection_id.hpp"
#include "quic_packet_num.hpp"
#include "quic_constants.hpp"

namespace quic {

enum class HeaderForm : bool {
    Long = 1,
    Short = 0,
};

//Packet numbers are divided into three spaces
enum class PacketNumberSpace : uint8_t {
    Initial,
    Handshake,
    AppData,
    MAX,
};

struct LongHeaderInvariant {
    QuicVersion _version;
    ConnectionId _srcConnId;
    ConnectionId _dstConnId;

    LongHeaderInvariant(QuicVersion ver, ConnectionId scid, ConnectionId dcid)
        :_version(ver),_srcConnId(std::move(scid)),_dstConnId(std::move(dcid)){
    }
};


enum class ProtectionType {
    Initial,
    Handshake,
    ZeroRtt,
    KeyPhaseZero,
    KeyPhaseOne,
};

constexpr uint8_t kHeaderFormMask = 0x80;
constexpr uint32_t kMaxPacketNumEncodingSize = 4;
constexpr uint32_t kNumInitialAckBlocksPerFrame = 32;

HeaderForm getHeaderForm(uint8_t headerValue);

struct LongHeader {
public:
    virtual ~LongHeader() = default;

    static constexpr uint8_t kFixedBitMask = 0x40;
    static constexpr uint8_t kPacketTypeMask = 0x30;
    static constexpr uint8_t kReservedBitsMask = 0x0c;
    static constexpr uint8_t kPacketNumLenMask = 0x03;
    static constexpr uint8_t kTypeBitsMask = 0x0F;

    static constexpr uint8_t kTypeShift = 4;
    enum class Types : uint8_t {
          Initial = 0x0,
          ZeroRtt = 0x1,
          Handshake = 0x2,
          Retry = 0x3,
    };

    // Note this is defined in the header so it is inlined for performance.
    static PacketNumberSpace typeToPacketNumberSpace(Types longHeaderType) {
        switch (longHeaderType) {
            case LongHeader::Types::Initial:
            case LongHeader::Types::Retry:
                return PacketNumberSpace::Initial;
            case LongHeader::Types::Handshake:
                return PacketNumberSpace::Handshake;
            case LongHeader::Types::ZeroRtt:
                return PacketNumberSpace::AppData;
        }
    }
    

    LongHeader(Types type, const ConnectionId& srcConnId, const ConnectionId& dstConnId, PacketNum packetNum, QuicVersion version, std::string token = std::string());
    LongHeader(Types type, LongHeaderInvariant invariant, std::string token = std::string());

    LongHeader(const LongHeader& other) = default;
    LongHeader(LongHeader&& other) = default;
    LongHeader& operator=(const LongHeader& other) = default;
    LongHeader& operator=(LongHeader&& other) = default;

    Types getHeaderType() const noexcept;
    const ConnectionId& getSourceConnId() const;
    const ConnectionId& getDestinationConnId() const;
    QuicVersion getVersion() const;
    
    // Note this is defined in the header so it is inlined for performance.
    PacketNumberSpace getPacketNumberSpace() const {
        return typeToPacketNumberSpace(_longHeaderType);
    }

    ProtectionType getProtectionType() const;
    bool hasToken() const;
    const std::string& getToken() const;
    // Note this is defined in the header so it is inlined for performance.
    PacketNum getPacketSequenceNum() const {
        return _packetSequenceNum;
    }

    void setPacketNumber(PacketNum packetNum);

 private:
    PacketNum _packetSequenceNum{0};
    Types _longHeaderType;
    LongHeaderInvariant _invariant;
    std::string _token;
};

}