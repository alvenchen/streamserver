#include "connection.hpp"

Connection::Connection(std::shared_ptr<ScopeRunner> scopeRunner, boost::asio::io_service &ioService, size_t bufferSize, size_t streamBufferSize){
    _socket = std::make_shared<tcp::socket>(ioService);
    _scopeRunner = scopeRunner;
    _timer = nullptr;

    _socketBuf = std::make_shared<boost::asio::streambuf>(bufferSize);
    _uuid = boost::uuids::random_generator()();
    _rtmp = std::make_shared<Rtmp>(streamBufferSize);
    _SocketBufUnconsumedLen = 0;
}

Connection::~Connection() noexcept{
    Close();
}

bool Connection::operator<(const Connection &r) const{
    error_code ec, ec_r;
    auto remoteEndpoint = _socket->remote_endpoint(ec);
    auto remoteEndpoint_r = r.GetSocket()->remote_endpoint(ec_r);

    if(ec || ec_r){
        auto localEndpoint = _socket->local_endpoint(ec);
        auto localEndpoint_r = r.GetSocket()->local_endpoint(ec_r);

        if(ec || ec_r){
            return _uuid < r.GetUUID();
        }else{
            if(localEndpoint.address() == localEndpoint_r.address()){
                return localEndpoint.port() < localEndpoint_r.port();
            }
            return localEndpoint.address() < localEndpoint_r.address();
        }
    }else{
        if(remoteEndpoint.address() == remoteEndpoint_r.address()){
            return remoteEndpoint.port() < remoteEndpoint_r.port();
        }
        return remoteEndpoint.address() < remoteEndpoint_r.address();
    }
}


void Connection::Close() noexcept{
    LOG::get(MAIN_LOG)->debug("conncection close");
    error_code ec;
    std::unique_lock<std::mutex> lock(_socketCloseMutex); // The following operations seems to be needed to run sequentially
    _socket->lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
    _socket->lowest_layer().close(ec);
}

//TODO 回调通知serverBase
void Connection::SetTimeout(long seconds, std::function<void(std::shared_ptr<Connection>)> func) {
    if(seconds == 0) {
        _timer = nullptr;
        return;
    }
    _timeoutCallback = func;

    _timer = std::unique_ptr<boost::asio::steady_timer>(new boost::asio::steady_timer(_socket->get_executor()));
    _timer->expires_from_now(std::chrono::seconds(seconds));

    auto self = this->shared_from_this();
    _timer->async_wait([self](const error_code &ec) {
        if(!ec){
            if(self->_timeoutCallback){
                self->_timeoutCallback(self);
            }
            self->Close();
        }
    });
}

void Connection::CancelTimeout() {
    if(_timer) {
        error_code ec;
        _timer->cancel(ec);
    }
}

asio::BOOST_ASIO_MUTABLE_BUFFER Connection::Prepare(){
    return _socketBuf->prepare(_socketBuf->max_size() - _SocketBufUnconsumedLen);
}

std::unique_ptr<ScopeRunner::AtomicAutoDec> Connection::Checking(){
    return _scopeRunner->Checking();
}

long Connection::OnRecvDatas(std::size_t bytesTransferred){

    _socketBuf->commit(bytesTransferred);
    
    long ret = _rtmp->OnRecvDatas(_socketBuf->data());

    if(ret >= 0){
        _SocketBufUnconsumedLen = _socketBuf->size() - (size_t)ret;

        _socketBuf->consume((size_t)ret);
    }

    return ret;
}
