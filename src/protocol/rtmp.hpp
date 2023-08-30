#pragma once

#include <stdint.h>
#include <vector>
#include <boost/asio.hpp>

const uint8_t RTMP_C0 = 0x03;

using namespace boost;

class RtmpChunk{
public:


};

class Rtmp{
public:
    Rtmp(std::size_t streamBufSize);

    /*
        check data format, return len of processed datas
    */
    long onRecvDatas(asio::streambuf::const_buffers_type bufs);

    int shouldSendDatas();

private:
    bool checkC0(asio::streambuf::const_buffers_type &bufs);
    bool checkC1(asio::streambuf::const_buffers_type &bufs);
    bool checkC2(asio::streambuf::const_buffers_type &bufs);
    bool createS0();
    bool createS1();
    bool createS2();

private:
    enum RTMP_STATUS{
        RTMP_INIT,
        RTMP_C0,
        RTMP_C1,
    };

    RTMP_STATUS _status;
    std::size_t _streamBufSize;
    //std::vector<char> _protocolCache;

};

