#pragma once
/* YP3 port: the layer is freestanding C++ (no libstdc++ headers), so the few
 * std names the game uses are provided here. */
#include <stddef.h>
#include <stdint.h>

namespace std {
    using ::size_t;
    using ::int16_t;
    using ::uint8_t;
    template <typename A, typename B> struct pair {
        A first;
        B second;
    };
    template <typename A, typename B> pair<A, B> make_pair(A a, B b) { return pair<A, B>{a, b}; }
    template <typename T> const T &min(const T &a, const T &b) { return b < a ? b : a; }
}
