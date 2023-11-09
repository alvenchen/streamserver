#include "server_state_machine.h"

namespace quic{

void onServerReadData(QuicServerConnectionState& connState, seastar::net::packet& data){
    switch (connState.state) {
        case ServerState::Open:
            onServerReadDataFromOpen(connState, data);
            return;
        case ServerState::Closed:
            onServerReadDataFromClosed(connState, data);
            return;
    }
}

void onServerReadDataFromOpen(QuicServerConnectionState& connState, seastar::net::packet& data){
    if(data.len() == 0){
        return;
    }
    bool firstPacketFromPeer = false;
    if(!connState.readCodec){
        firstPacketFromPeer = true;

        auto initialByte = data.get_header(0, 1);

        auto parsedLongHeader = parseLongHeaderInvariant(initialByte, 1, data);
        if(!parsedLongHeader){
            if(connState.qLogger){
                connState.qLogger->addPacketDrop(0,PacketDropReason(PacketDropReason::PARSE_ERROR_LONG_HEADER_INITIAL)._to_string());
            }
            QUIC_STATS(connState.statsCallback, onPacketDropped, PacketDropReason::PARSE_ERROR_LONG_HEADER_INITIAL);
            return;
        }

        QuicVersion version = parsedLongHeader->invariant.version;
        if (version == QuicVersion::VERSION_NEGOTIATION) {
            if (conn.qLogger) {
                conn.qLogger->addPacketDrop(0, PacketDropReason(PacketDropReason::INVALID_PACKET_VN)._to_string());
            }
            QUIC_STATS(conn.statsCallback, onPacketDropped, PacketDropReason::INVALID_PACKET_VN);
            return;
        }

        const auto& clientConnectionId = parsedLongHeader->invariant.srcConnId;
        const auto& initialDestinationConnectionId = parsedLongHeader->invariant.dstConnId;
        if (initialDestinationConnectionId.size() < kDefaultConnectionIdSize) {
            //VLOG(4) << "Initial connectionid too small";
            if (conn.qLogger) {
                conn.qLogger->addPacketDrop(0, PacketDropReason(PacketDropReason::INITIAL_CONNID_SMALL)._to_string());
            }
            QUIC_STATS(conn.statsCallback, onPacketDropped, PacketDropReason::INITIAL_CONNID_SMALL);
            return;
        }

        //TODO CHECK(conn.connIdAlgo)

        auto newServerConnIdData = connState.createAndAddNewSelfConnId();
        connState.serverConnectionId = newServerConnIdData->connId;

        auto customTransportParams = setSupportedExtensionTransportParameters(conn);
    } // end of !readCodec
}

void onServerReadDataFromClosed(QuicServerConnectionState& connState, seastar::net::packet& data){

}



}