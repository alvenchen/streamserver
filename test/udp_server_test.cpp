/*
for test:
A udp server using seastar framework randomly change connection thread

*/

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

class udp_server_test {
private:
    udp_channel _chan;
    timer<> _statsTimer;
    uint64_t _nSent {};
public:
    void start(uint16_t port) {
        fmt::print("start shard: {:d}\n", this_shard_id());
        ipv4_addr listen_addr{port};
        _chan = make_udp_channel(listen_addr);

        _statsTimer.set_callback([this] {
            fmt::print("thread_id:{}, shard_id:{}, pps:{}\n", std::this_thread::get_id(), this_shard_id(), _nSent);
            _nSent = 0;
        });
        _statsTimer.arm_periodic(1s);

        // Run server in background.
        (void)keep_doing([this] {
            return _chan.receive().then([this] (udp_datagram dgram) {
                fmt::print("receive thread_id:{}, shard_id:{}\n", std::this_thread::get_id(), this_shard_id());
                return _chan.send(dgram.get_src(), std::move(dgram.get_data())).then([this] {
                    fmt::print("send thread_id:{}, shard_id:{}\n", std::this_thread::get_id(), this_shard_id());
                    _nSent++;
                });
            });
        });
    }
    // FIXME: we should properly tear down the service here.
    future<> stop() {
        return make_ready_future<>();
    }
};

int run(){

    return 0;
}

int main(int ac, char** av) {
    app_template app;

    app.add_options()("port", bpo::value<uint16_t>()->default_value(443), "UDP server port") ;
    std::cout << "start\n";

    app.run_deprecated(ac, av, [&]{   
        
        fmt::print("Total cpus: {:d}\n", smp::count);
        fmt::print("thread affinity: {:d}\n", app.options().smp_opts.thread_affinity);

        auto& opts = app.configuration();
        auto& port = opts["port"].as<uint16_t>();

        auto server = new distributed<udp_server_test>;

        (void)server->start().then([server = std::move(server), port] () mutable {
            engine().at_exit([server] {
                return server->stop();
            });
            return server->invoke_on_all(&udp_server_test::start, port);
        }).then([port] {
            std::cout << "Seastar UDP server listening on port " << port << " ...\n";
        });
    
    });
}