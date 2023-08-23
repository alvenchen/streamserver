#pragma once


#include <cstdint>
#include <string>

class Config{
public:
    Config() noexcept {};

public:
    
    uint16_t Port = 1935;
    
    uint32_t ThreadPoolSize = 4;
    
    //seconds
    long TimeoutRequest = 5;
    long TimeoutContent = 300;
    
    uint32_t MaxRecvbufSize = 1 * 1024 * 1024;
    uint32_t MaxStreambufSize = 10 * 1024 * 1024;

    std::string Address;

    bool ReuseAddress = true;
};
