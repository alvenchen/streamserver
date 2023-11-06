#include "server_state_machine.h"


void onServerReadData(QuicServerConnectionState& connState, seastar::packet data){
    switch (connState.state) {
        case ServerState::Open:
            onServerReadDataFromOpen(conn, data);
            return;
        case ServerState::Closed:
            onServerReadDataFromClosed(conn, data);
            return;
    }
}

void onServerReadDataFromOpen(QuicServerConnectionState& conn, seastar::packet& data){
    if(data.len == 0){
        return;
    }
    bool firstPacketFromPeer = false;
    if(!conn.readCodec){
        firstPacketFromPeer = true;
    }
}

void onServerReadDataFromClosed(QuicServerConnectionState& conn, seastar::packet& data){

}