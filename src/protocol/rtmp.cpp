#include "rtmp.hpp"


Rtmp::Rtmp(std::size_t streamBufSize){
    _status = RTMP_INIT;
    _streamBufSize = streamBufSize;
}


long Rtmp::OnRecvDatas(asio::streambuf::const_buffers_type bufs){

    switch (_status)
    {
    case RTMP_INIT:
        //_protocolCache.insert(_protocolCache.end(), asio::buffers_begin(bufs), asio::buffers_end(bufs));
        
            if(!CheckC0(bufs)){
                return -1;
            }
        }

        for(auto it=asio::buffers_begin(bufs); it!=asio::buffers_end(bufs); ++it){
            LOG::get(MAIN_LOG)->debug("{:c}", *it);
        }
        break;
    
    default:
        break;
    }

    long consume = asio::buffers_end(bufs) - asio::buffers_begin(bufs);
    LOG::get(MAIN_LOG)->debug("size: {:d}", consume);

    return consume;
}

bool Rtmp::CheckC0(asio::streambuf::const_buffers_type &bufs){
    auto it = asio::buffers_begin(bufs);
    if(it == asio::buffers_end(bufs)){
        return false;
    }
    if(*it == RTMP_C0){
        return true;
    }else{
        //TODO : other self defined private protocol, for example: 0xf3
        return false;
    }
    return false;
}

bool Rtmp::CreateS0(){

    return true;
}
