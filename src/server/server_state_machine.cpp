#include "server_state_machine.h"

namespace quic{



folly::Optional<ConnectionIdData> QuicServerConnectionState::createAndAddNewSelfConnId() {
    // Should be set right after server transport construction.
    //CHECK(connIdAlgo);
    //CHECK(serverConnIdParams);
    //CHECK(transportSettings.statelessResetTokenSecret);

    //TODO connIdAlgo

    StatelessResetGenerator generator(transportSettings.statelessResetTokenSecret.value(), serverAddr.getFullyQualified());

    // The default connectionId algo has 36 bits of randomness.
    auto encodedCid = connIdAlgo->encodeConnectionId(*serverConnIdParams);
    size_t encodedTimes = 1;
    while (encodedCid && connIdRejector && connIdRejector->rejectConnectionId(*encodedCid) && encodedTimes < kConnIdEncodingRetryLimit) {
        encodedCid = connIdAlgo->encodeConnectionId(*serverConnIdParams);
        encodedTimes++;
    }
    //LOG_IF(ERROR, encodedTimes == kConnIdEncodingRetryLimit) << "Quic CIDRejector rejected all conneectionIDs";
    if (encodedCid.hasError()) {
        return folly::none;
    }
    QUIC_STATS(statsCallback, onConnectionIdCreated, encodedTimes);
    auto newConnIdData = ConnectionIdData{*encodedCid, nextSelfConnectionIdSequence++};
    newConnIdData.token = generator.generateToken(newConnIdData.connId);
    selfConnectionIds.push_back(newConnIdData);

    return newConnIdData;
}


/*

*/
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

void onServerReadDataFromOpen(QuicServerConnectionState& conn, seastar::net::packet& data){
    if(data.len() == 0){
        return;
    }
    bool firstPacketFromPeer = false;
    if(!conn.readCodec){
        firstPacketFromPeer = true;

        auto initialByte = data.get_header(0, 1);

        auto parsedLongHeader = parseLongHeaderInvariant(initialByte, 1, data);
        if(!parsedLongHeader){
            if(conn.qLogger){
                conn.qLogger->addPacketDrop(0,PacketDropReason(PacketDropReason::PARSE_ERROR_LONG_HEADER_INITIAL)._to_string());
            }
            QUIC_STATS(conn.statsCallback, onPacketDropped, PacketDropReason::PARSE_ERROR_LONG_HEADER_INITIAL);
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

        auto newServerConnIdData = conn.createAndAddNewSelfConnId();
        conn.serverConnectionId = newServerConnIdData->connId;

        auto customTransportParams = setSupportedExtensionTransportParameters(conn);

        QUIC_STATS(conn.statsCallback, onStatelessReset);

        conn.serverHandshakeLayer->accept(
            std::make_shared<ServerTransportParametersExtension>(
            version,
            conn.transportSettings.advertisedInitialConnectionWindowSize,
            conn.transportSettings.advertisedInitialBidiLocalStreamWindowSize,
            conn.transportSettings.advertisedInitialBidiRemoteStreamWindowSize,
            conn.transportSettings.advertisedInitialUniStreamWindowSize,
            conn.transportSettings.advertisedInitialMaxStreamsBidi,
            conn.transportSettings.advertisedInitialMaxStreamsUni,
            conn.transportSettings.idleTimeout,
            conn.transportSettings.ackDelayExponent,
            conn.transportSettings.maxRecvPacketSize,
            *newServerConnIdData->token,
            conn.serverConnectionId.value(),
            initialDestinationConnectionId,
            customTransportParams));
            
    conn.transportParametersEncoded = true;
    const CryptoFactory& cryptoFactory =
        conn.serverHandshakeLayer->getCryptoFactory();
    conn.readCodec = std::make_unique<QuicReadCodec>(QuicNodeType::Server);
    conn.readCodec->setInitialReadCipher(cryptoFactory.getClientInitialCipher(
        initialDestinationConnectionId, version));
    conn.readCodec->setClientConnectionId(clientConnectionId);
    conn.readCodec->setServerConnectionId(*conn.serverConnectionId);
    if (conn.qLogger) {
      conn.qLogger->setScid(conn.serverConnectionId);
      conn.qLogger->setDcid(initialDestinationConnectionId);
    }
    conn.readCodec->setCodecParameters(CodecParameters(
        conn.peerAckDelayExponent,
        version,
        conn.transportSettings.maybeAckReceiveTimestampsConfigSentToPeer));
    conn.initialWriteCipher = cryptoFactory.getServerInitialCipher(
        initialDestinationConnectionId, version);

    conn.readCodec->setInitialHeaderCipher(
        cryptoFactory.makeClientInitialHeaderCipher(
            initialDestinationConnectionId, version));
    conn.initialHeaderCipher = cryptoFactory.makeServerInitialHeaderCipher(
        initialDestinationConnectionId, version);
    conn.peerAddress = conn.originalPeerAddress;
    } // end of !readCodec
}

void onServerReadDataFromClosed(QuicServerConnectionState& conn, seastar::net::packet& data){

}


std::vector<TransportParameter> setSupportedExtensionTransportParameters(QuicServerConnectionState& conn) {
    std::vector<TransportParameter> customTransportParams;
    const auto& ts = conn.transportSettings;
    if (ts.datagramConfig.enabled) {
        CustomIntegralTransportParameter maxDatagramFrameSize(
            static_cast<uint64_t>(TransportParameterId::max_datagram_frame_size), 
            conn.datagramState.maxReadFrameSize);

        customTransportParams.push_back(maxDatagramFrameSize.encode());
    }

    if (ts.advertisedMaxStreamGroups > 0) {
        CustomIntegralTransportParameter streamGroupsEnabledParam(
            static_cast<uint64_t>(TransportParameterId::stream_groups_enabled),
            ts.advertisedMaxStreamGroups);

        if (!setCustomTransportParameter(streamGroupsEnabledParam, customTransportParams)) {
            //LOG(ERROR) << "failed to set stream groups enabled transport parameter";
        }
    }

    CustomIntegralTransportParameter ackReceiveTimestampsEnabled(
        static_cast<uint64_t>(TransportParameterId::ack_receive_timestamps_enabled),
        ts.maybeAckReceiveTimestampsConfigSentToPeer.has_value() ? 1 : 0);
    customTransportParams.push_back(ackReceiveTimestampsEnabled.encode());

    if (ts.maybeAckReceiveTimestampsConfigSentToPeer.has_value()) {
        CustomIntegralTransportParameter maxReceiveTimestampsPerAck(
            static_cast<uint64_t>(TransportParameterId::max_receive_timestamps_per_ack),
            ts.maybeAckReceiveTimestampsConfigSentToPeer->maxReceiveTimestampsPerAck);
        customTransportParams.push_back(maxReceiveTimestampsPerAck.encode());

        CustomIntegralTransportParameter receiveTimestampsExponent(
            static_cast<uint64_t>(TransportParameterId::receive_timestamps_exponent),
            ts.maybeAckReceiveTimestampsConfigSentToPeer->receiveTimestampsExponent);
        customTransportParams.push_back(receiveTimestampsExponent.encode());
    }

    if (ts.minAckDelay) {
        CustomIntegralTransportParameter minAckDelay(
            static_cast<uint64_t>(TransportParameterId::min_ack_delay),
            ts.minAckDelay.value().count());
        customTransportParams.push_back(minAckDelay.encode());
    }

    if (ts.advertisedKnobFrameSupport) {
        CustomIntegralTransportParameter knobFrameSupport(static_cast<uint64_t>(TransportParameterId::knob_frames_supported), 1);
        customTransportParams.push_back(knobFrameSupport.encode());
    }

    return customTransportParams;
}


}