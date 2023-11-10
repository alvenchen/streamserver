
#pragma once

#include "state/state_data.h"
#include "protocol/quic_constants.hpp"
#include <seastar/net/packet.hh>


namespace quic{

enum ServerState {
    Open,
    Closed,
};


struct QuicServerConnectionState : public QuicConnectionStateBase {
    ~QuicServerConnectionState() override = default;
    ServerState state;

    // Server address of VIP. Currently used as input for stateless reset token.
    // TODO set serverAddr
    folly::SocketAddress serverAddr;

    ServerHandshake* serverHandshakeLayer;

    QuicServerConnectionState():QuicConnectionStateBase(QuicNodeType::Server){
        state = ServerState::Open;

    }

    folly::Optional<ConnectionIdData> createAndAddNewSelfConnId() override;
    
    // Parameters to generate server chosen connection id
    folly::Optional<ServerConnectionIdParams> serverConnIdParams;

};


void onServerReadData(QuicServerConnectionState& conn, seastar::net::packet& data);
void onServerReadDataFromOpen(QuicServerConnectionState& conn, seastar::net::packet& data);
void onServerReadDataFromClosed(QuicServerConnectionState& conn, seastar::net::packet& data);

std::vector<TransportParameter> setSupportedExtensionTransportParameters(QuicServerConnectionState& conn);

}