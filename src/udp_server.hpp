/*

*/
#pragma once

#include <iostream>
#include <seastar/core/reactor.hh>
#include <seastar/core/distributed.hh>
#include <seastar/core/app-template.hh>
#include <boost/program_options.hpp>
#include <fmt/printf.h>
#include "common/common.hpp"

using namespace seastar;
using namespace net;

namespace bpo = boost::program_options;


class UDPServer {
private:
    udp_channel _listenChan;
    timer<> _statsTimer;
    uint64_t _nSent {};
public:
    UDPServer();
    void Start(uint16_t port);

    future<> Stop();

private:
    int handleUnknowPacket(packet data);
};