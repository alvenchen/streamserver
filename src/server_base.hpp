#ifndef SERVER_BASE_HPP
#define SERVER_BASE_HPP

#include "utility.hpp"
#include "config.hpp"

#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_set>

#include <openssl/aes.h>
#include <boost/asio/steady_timer.hpp>
#include <thread>
#include "connection.hpp"
#include "scope_runner.hpp"

class ServerBase : public std::enable_shared_from_this<ServerBase>{

public:
    ServerBase();
    virtual ~ServerBase() {};

    int start();
    int stop();
protected:
    int bind();
    int accept();
    int innerRun();
    std::shared_ptr<Connection> createConnection(boost::asio::io_service &ioService);

    void read(const std::shared_ptr<Connection> &session);
    void removeConn(const std::shared_ptr<Connection> &conn);

protected:
    Config _config;

    std::unique_ptr<tcp::acceptor> _acceptor;
    std::shared_ptr<boost::asio::io_service> _ioService;
    std::shared_ptr<std::unordered_set<Connection *>> _connections;

    //map publish and subscribe
    std::shared_ptr<std::map<Connection *, Connection *>> _connMap;

    std::shared_ptr<std::mutex> _connectionsMutex;
    std::shared_ptr<ScopeRunner> _scopeRunner;
};

#endif