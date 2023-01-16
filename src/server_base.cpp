#include "server_base.hpp"

/* public */

ServerBase::ServerBase(){
    _ioService = std::make_shared<boost::asio::io_service>();
    _acceptor = std::make_unique<tcp::acceptor>(*_ioService);
    _connections =  std::make_shared<std::unordered_set<Connection *>>();
    _connectionsMutex = std::make_shared<std::mutex>();
    _scopeRunner = std::make_shared<ScopeRunner>();

    _connMap = std::make_shared<std::map<Connection*, Connection*>>();
}

int ServerBase::start(){
    bind();
    accept();
    innerRun();

    return 0;
}

int ServerBase::stop(){
    error_code ec;
    _acceptor->close(ec);
    {
        std::unique_lock<std::mutex> lock(*_connectionsMutex);
        for(auto &connection : *_connections){
            connection->Close();
        }
        _connections->clear();
    }
    _ioService->stop();

    return 0;
}


/* protected */

int ServerBase::bind(){
    try{
        tcp::endpoint endpoint;
        if(_config.address.size() > 0)
            endpoint = tcp::endpoint(address::from_string(_config.address), _config.port);
        else
            endpoint = tcp::endpoint(tcp::v4(), _config.port);

        _acceptor->open(endpoint.protocol());
        _acceptor->set_option(boost::asio::socket_base::reuse_address(_config.reuse_address));
        _acceptor->bind(endpoint);

        //after bind
    }catch(...){
        abort();
        return -1;
    }
    
    return 0;
}

int ServerBase::accept(){
    _acceptor->listen();

    auto connection = createConnection(*_ioService);
    _acceptor->async_accept(*(connection->GetSocket()), [this, connection](const error_code &ec) {
        auto lock = connection->Checking();
        if(!lock){
            return;
        }

        //set option should be after socket open
        boost::asio::ip::tcp::no_delay option(true);
        connection->GetSocket()->set_option(option);

        // Immediately start accepting a new connection (unless io_service has been stopped)
        if(ec != boost::asio::error::operation_aborted){
            this->accept();
        }

        if(!ec) {
            this->read(connection);
        }
        else{
            //TODO
            LOG::get(MAIN_LOG)->warn("accept error : {:d}\n", ec.value());
        }

      });

    return 0;
}

int ServerBase::innerRun(){
    _ioService->run();

    return 0;
}

std::shared_ptr<Connection> ServerBase::createConnection(boost::asio::io_service &ioService){
    auto connection = std::make_shared<Connection>(_scopeRunner, ioService, _config.max_recvbuf_size, _config.max_streambuf_size);

    std::unique_lock<std::mutex> lock(*_connectionsMutex);

    auto it = _connections->find(connection.get());
    if(it != _connections->end()){
        _connections->erase(it);
    }
    _connections->emplace(connection.get());

    return connection;
}

void ServerBase::read(const std::shared_ptr<Connection> &conn){
    
    std::function<void(std::shared_ptr<Connection>)> func = std::bind(&ServerBase::removeConn, this, std::placeholders::_1);
    conn->SetTimeout(_config.timeout_request, func);

    conn->GetSocket()->async_read_some(conn->Prepare(), [this, conn](const error_code &ec, std::size_t bytesTransferred){
        conn->CancelTimeout();

        auto lock = conn->Checking();
        if(!lock){
            LOG::get(MAIN_LOG)->debug("connection break, read exit");
            return;
        }

        if(!ec){
            
            long ret = conn->OnRecvDatas(bytesTransferred);
            if(ret < 0){
                LOG::get(MAIN_LOG)->debug("OnRecvDatas error, read exit");
                removeConn(conn);
                return;
            }
            
            
            this->read(conn);
        }else{
            //TODO reconncect?
            removeConn(conn);
        }
    });
}

void ServerBase::removeConn(const std::shared_ptr<Connection> &conn){
    std::unique_lock<std::mutex> lock(*_connectionsMutex);
    
    _connections->erase(conn.get());
}