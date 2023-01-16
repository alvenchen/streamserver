#ifndef SCOPE_RUNNER_HPP
#define SCOPE_RUNNER_HPP

#include <atomic>

// Makes it possible to for instance cancel Asio handlers without stopping asio::io_service
class ScopeRunner{
    std::atomic<long> _count;

public:
    class AtomicAutoDec{
        friend class ScopeRunner;
        std::atomic<long> &_cnt;
        AtomicAutoDec(std::atomic<long> &c) noexcept : _cnt(c){};
        AtomicAutoDec &operator=(const AtomicAutoDec &) = delete;
        AtomicAutoDec(const AtomicAutoDec&) = delete;

        public:
            ~AtomicAutoDec(){
                //default memory_order_seq_cst ensures Sequential consistency
                _cnt.fetch_sub(1);
            };
    };

public:
    ScopeRunner() : _count(0){};
    
    std::unique_ptr<AtomicAutoDec> Checking(){
        long expected = _count;
        while(expected >= 0 && !_count.compare_exchange_weak(expected, expected + 1))
            spin_loop_pause();

        if(expected < 0)
            return nullptr;
        else
            return std::unique_ptr<AtomicAutoDec>(new AtomicAutoDec(_count));
    };
    void StopScope(){
        long expected = 0;
        while(!_count.compare_exchange_weak(expected, -1)){
            if(expected < 0)
                return;
            expected = 0;
            spin_loop_pause();
        }
    }
};


#endif