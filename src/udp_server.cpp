#include "udp_server.hpp"

UDPServer::UDPServer(){

}

void UDPServer::Start(uint16_t port){
    ipv4_addr listen_addr(port);
    _listenChan = make_udp_channel(listen_addr);

    // Run server in background.
    (void)keep_doing([this] {
        return _listenChan.receive().then([this] (udp_datagram dgram) {
            //TODO  dispatch
            
            HandleUnknowPacket(std::move(dgram.get_data()));
        });
    });
}

future<> UDPServer::Stop(){
    //FIME ME
    return make_ready_future<>();
}


/*
private function
*/

int UDPServer::HandleUnknowPacket(packet p){
    p.get_header();

    return 0;
}