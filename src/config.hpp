#ifndef CONFIG_HPP
#define CONFIG_HPP


 class Config{
public:
    Config() noexcept {};

public:
    
    unsigned short port = 1935;
    
    int thread_pool_size = 1;
    
    //seconds
    long timeout_request = 5;
    long timeout_content = 300;
    
    std::size_t max_recvbuf_size = 1 * 1024 * 1024;
    std::size_t max_streambuf_size = 10 * 1024 * 1024;

    std::string address;

    bool reuse_address = true;
};

#endif