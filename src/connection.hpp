#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "utility.hpp"
#include "scope_runner.hpp"
#include "protocol/rtmp.hpp"

/* tcp connection */

// enable_shared_from_this : because of self->Close
class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(std::shared_ptr<ScopeRunner> scopeRunner, boost::asio::io_service &ioService, size_t bufferSize, size_t streamBufferSize);
    ~Connection() noexcept;

    bool operator < (const Connection& r) const;
    Connection& operator = (const Connection& r) const;

    void Close() noexcept;

    void SetTimeout(long seconds, std::function<void(std::shared_ptr<Connection>)> func);

    void CancelTimeout();

    long OnRecvDatas(std::size_t bytesTransferred);

    asio::BOOST_ASIO_MUTABLE_BUFFER Prepare();

    uuid GetUUID() const {return _uuid;};
    std::shared_ptr<tcp::socket> GetSocket() const {return _socket;};
    std::shared_ptr<boost::asio::streambuf> GetBuffer() const {return _socketBuf;};
    std::unique_ptr<ScopeRunner::AtomicAutoDec> Checking();
protected:
    std::shared_ptr<tcp::socket> _socket;
    std::shared_ptr<ScopeRunner>  _scopeRunner;
    std::mutex _socketCloseMutex;
    std::unique_ptr<boost::asio::steady_timer> _timer;

    std::shared_ptr<boost::asio::streambuf> _socketBuf;
    std::size_t _SocketBufUnconsumedLen;

    std::shared_ptr<Rtmp> _rtmp;

    std::function<void(std::shared_ptr<Connection>)> _timeoutCallback;

    uuid _uuid;
};

#endif