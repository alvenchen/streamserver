/*
 
*/

#pragma once


#include "../common/common.hpp"
#include "quic_connection_id.hpp"
#include "quic_packet_num.hpp"
#include "quic_constants.hpp"
#include "../folly/io/Cursor.h"

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
    QuicVersion version;
    ConnectionId srcConnId;
    ConnectionId dstConnId;

    LongHeaderInvariant(QuicVersion ver, ConnectionId scid, ConnectionId dcid)
        :version(ver),srcConnId(std::move(scid)),dstConnId(std::move(dcid)){
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

    bool hasToken() const;
    const std::string& getToken() const;
    // Note this is defined in the header so it is inlined for performance.
    PacketNum getPacketSequenceNum() const {
        return _packetSequenceNum;
    }

    void setPacketNumber(PacketNum packetNum);

    ProtectionType getProtectionType() const;

 private:
    PacketNum _packetSequenceNum{0};
    Types _longHeaderType;
    LongHeaderInvariant _invariant;
    std::string _token;
};



struct ShortHeaderInvariant {
    ConnectionId destinationConnId;

    explicit ShortHeaderInvariant(ConnectionId dcid);
};



struct ShortHeader {
public:
    virtual ~ShortHeader() = default;

    // There is also a spin bit which is 0x20 that we don't currently implement.
    static constexpr uint8_t kFixedBitMask = 0x40;
    static constexpr uint8_t kReservedBitsMask = 0x18;
    static constexpr uint8_t kKeyPhaseMask = 0x04;
    static constexpr uint8_t kPacketNumLenMask = 0x03;
    static constexpr uint8_t kTypeBitsMask = 0x1F;

    /**
     * The constructor for reading a packet.
     */
    ShortHeader(ProtectionType protectionType, ConnectionId connId);

    /**
     * The constructor for writing a packet.
     */
    ShortHeader(ProtectionType protectionType, ConnectionId connId, PacketNum packetNum);

    PacketNumberSpace getPacketNumberSpace() const {
        return PacketNumberSpace::AppData;
    }
    PacketNum getPacketSequenceNum() const {
        return packetSequenceNum_;
    }
    const ConnectionId& getConnectionId() const;

    void setPacketNumber(PacketNum packetNum);

    ProtectionType getProtectionType() const;
private:
    ShortHeader() = delete;
    //bool readInitialByte(uint8_t initalByte);
    //bool readConnectionId(folly::io::Cursor& cursor);
    //bool readPacketNum(PacketNum largestReceivedPacketNum, folly::io::Cursor& cursor);

private:
    PacketNum packetSequenceNum_{0};
    ProtectionType protectionType_;
    ConnectionId connectionId_;
};



struct PacketHeader {
    ~PacketHeader();

    /* implicit */ PacketHeader(LongHeader&& longHeader);
    /* implicit */ PacketHeader(ShortHeader&& shortHeader);

    PacketHeader(PacketHeader&& other) noexcept;
    PacketHeader(const PacketHeader& other);

    PacketHeader& operator=(PacketHeader&& other) noexcept;
    PacketHeader& operator=(const PacketHeader& other);

    LongHeader* asLong();
    ShortHeader* asShort();

    const LongHeader* asLong() const;
    const ShortHeader* asShort() const;

    // Note this is defined in the header so it is inlined for performance.
    PacketNum getPacketSequenceNum() const {
        switch (headerForm_) {
            case HeaderForm::Long:
            return longHeader.getPacketSequenceNum();
            case HeaderForm::Short:
            return shortHeader.getPacketSequenceNum();
            default:
            folly::assume_unreachable();
        }
    }
    HeaderForm getHeaderForm() const;
    ProtectionType getProtectionType() const;
    // Note this is defined in the header so it is inlined for performance.
    PacketNumberSpace getPacketNumberSpace() const {
        switch (headerForm_) {
            case HeaderForm::Long:
            return longHeader.getPacketNumberSpace();
            case HeaderForm::Short:
            return shortHeader.getPacketNumberSpace();
            default:
            folly::assume_unreachable();
        }
    }

private:
    void destroyHeader();

    union {
        LongHeader longHeader;
        ShortHeader shortHeader;
    };

    HeaderForm headerForm_;
};




}