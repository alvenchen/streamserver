/*
naming rule :
    functions : little camel-case
    class/struct : big camel-case


*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <type_traits>
#include <stdexcept>
#include <cstring>
#include <utility>

using namespace std::chrono_literals;

namespace common{

    template <typename Dst, typename Src>
    constexpr std::make_signed_t<Dst> bits_to_signed(Src const s) {
        return static_cast<Dst>(std::make_signed_t<Src>(s));
    }

    template <typename Dst, typename Src>
    constexpr std::make_unsigned_t<Dst> bits_to_unsigned(Src const s) {
        return static_cast<Dst>(std::make_unsigned_t<Src>(s));
    }

    // TLDR: Prefer using operator< for ordering. And when
    // a and b are equivalent objects, we return b to make
    // sorting stable.
    // See http://stepanovpapers.com/notes.pdf for details.
    template <typename T, typename... Ts>
    constexpr T constexprMax(T a, Ts... ts) {
        T list[] = {ts..., a}; // 0-length arrays are illegal
        for (auto i = 0u; i < sizeof...(Ts); ++i) {
            a = list[i] < a ? a : list[i];
        }
        return a;
    }

    template <typename T>
    inline constexpr unsigned int findLastSet(T const v){
        using U0 = unsigned int;
        using U1 = unsigned long int;
        using U2 = unsigned long long int;
        
        static_assert(sizeof(T) <= sizeof(U2), "over-sized type");
        static_assert(std::is_integral<T>::value, "non-integral type");
        static_assert(!std::is_same<T, bool>::value, "bool type");

        // If X is a power of two X - Y = 1 + ((X - 1) ^ Y). Doing this transformation
        // allows GCC to remove its own xor that it adds to implement clz using bsr.
        // clang-format off
        constexpr auto size = constexprMax(sizeof(T), sizeof(U0));
        return v ? 1u + static_cast<unsigned int>((8u * size - 1u) ^ (
            sizeof(T) <= sizeof(U0) ? __builtin_clz(bits_to_unsigned<U0>(v)) :
            sizeof(T) <= sizeof(U1) ? __builtin_clzl(bits_to_unsigned<U1>(v)) :
            sizeof(T) <= sizeof(U2) ? __builtin_clzll(bits_to_unsigned<U2>(v)) :
            0)) : 0u;
    }


}