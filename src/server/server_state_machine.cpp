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
    }
}

void onServerReadDataFromClosed(QuicServerConnectionState& connState, seastar::net::packet& data){

}



}