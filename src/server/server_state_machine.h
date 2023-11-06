
#pragma once

#include "../state/state_data.h"
#include "../protocol/quic_constants.hpp"
#include <seastar/net/packet.hh>


namespace quic{

enum ServerState {
    Open,
    Closed,
};


struct QuicServerConnectionState : public QuicConnectionStateBase {
    ~QuicServerConnectionState() override = default;
    ServerState state;

    QuicServerConnectionState():QuicConnectionStateBase(QuicNodeType::Server){
        state = ServerState::Open;

    }

};


void onServerReadData(QuicServerConnectionState& connState, seastar::net::packet& data);
void onServerReadDataFromOpen(QuicServerConnectionState& conn, seastar::net::packet& data);
void onServerReadDataFromClosed(QuicServerConnectionState& conn, seastar::net::packet& data);

}