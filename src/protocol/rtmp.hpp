#ifndef RTMP_HPP
#define RTMP_HPP

#include "utility.hpp"
#include <stdint.h>
#include <vector>

const uint8_t RTMP_C0 = 0x03;


class RtmpChunk{
public:


};

class Rtmp{
public:
    Rtmp(std::size_t streamBufSize);

    /*
        check data format, return len of processed datas
    */
    long OnRecvDatas(asio::streambuf::const_buffers_type bufs);

    int ShouldSendDatas();

private:
    bool CheckC0(asio::streambuf::const_buffers_type &bufs);
    bool CheckC1(asio::streambuf::const_buffers_type &bufs);
    bool CheckC2(asio::streambuf::const_buffers_type &bufs);
    bool CreateS0();
    bool CreateS1();
    bool CreateS2();

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


#endif