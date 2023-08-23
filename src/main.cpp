
#include <seastar/core/reactor.hh>
#include <seastar/core/distributed.hh>
#include <seastar/core/app-template.hh>
#include <udp_server.hpp>
#include <seastar/util/log.hh>

using namespace seastar;
using namespace net;
using namespace std::chrono_literals;

namespace bpo = boost::program_options;

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
            return server->invoke_on_all(&udp_server::Start, port);
        }).then([port] {
            std::cout << "Seastar UDP server listening on port " << port << " ...\n";
        });
    
    });

    return 0;
}