/*
for test:
A udp server using seastar framework randomly change connection thread

*/

#include <iostream>
#include <seastar/core/reactor.hh>
#include <seastar/core/distributed.hh>
#include <seastar/core/app-template.hh>
#include <boost/program_options.hpp>

using namespace seastar;
using namespace net;
using namespace std::chrono_literals;

namespace bpo = boost::program_options;

class udp_server {
private:
    udp_channel _chan;
    timer<> _stats_timer;
    uint64_t _n_sent {};
public:
    void start(uint16_t port) {
        ipv4_addr listen_addr{port};
        _chan = make_udp_channel(listen_addr);

        _stats_timer.set_callback([this] {
            std::cout << "Out: " << _n_sent << " pps" << std::endl;
            _n_sent = 0;
        });
        _stats_timer.arm_periodic(1s);

        // Run server in background.
        (void)keep_doing([this] {
            return _chan.receive().then([this] (udp_datagram dgram) {
                return _chan.send(dgram.get_src(), std::move(dgram.get_data())).then([this] {
                    _n_sent++;
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
        auto& opts = app.configuration();
        auto& port = opts["port"].as<uint16_t>();

        auto server = new distributed<udp_server>;

        (void)server->start().then([server = std::move(server), port] () mutable {
            engine().at_exit([server] {
                return server->stop();
            });
            return server->invoke_on_all(&udp_server::start, port);
        }).then([port] {
            std::cout << "Seastar UDP server listening on port " << port << " ...\n";
        });
    
    });
}