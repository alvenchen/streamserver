#include "server_base.hpp"
#include <signal.h>
using namespace std;

int main() {
    const auto LOGGER = LOG::rotating_logger_mt(MAIN_LOG, "logs/mainlog", LOG_MAX_SIZE, LOG_MAX_FLLES);
    LOGGER->set_level(LOG::level::debug);

    LOG::get(MAIN_LOG)->info("\n\nmain started");
    ServerBase server;

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = [](int s){
        LOG::get(MAIN_LOG)->info("receive signal:{:d}", s);
        LOG::get(MAIN_LOG)->info("main ended");
        LOG::drop_all();
        exit(0);
    };
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    std::thread server_thread([&server]() {
        server.start();
    });
    server_thread.join();


    return 0;
}