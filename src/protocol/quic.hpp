#include <cstdint>


namespace quic{
    using StreamId = uint64_t;
    using StreamGroupId = uint64_t;

    template <class T, size_t N>
    using SmallVec = std::vector<T>;

}

