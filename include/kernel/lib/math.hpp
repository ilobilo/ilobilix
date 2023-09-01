// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <cstdint>

extern uintptr_t hhdm_offset;

constexpr inline bool ishh(auto a)
{
    return uintptr_t(a) >= hhdm_offset;
}

template<typename Type>
constexpr inline Type tohh(Type a)
{
    return ishh(a) ? a : Type(uintptr_t(a) + hhdm_offset);
}

template<typename Type>
constexpr inline Type fromhh(Type a)
{
    return !ishh(a) ? a : Type(uintptr_t(a) - hhdm_offset);
}

constexpr inline auto align_down(auto n, auto a)
{
    return (n & ~(a - 1));
}

constexpr inline auto align_up(auto n, auto a)
{
    return align_down(n + a - 1, a);
}

constexpr inline auto div_roundup(auto n, auto a)
{
    return align_down(n + a - 1, a) / a;
}

constexpr inline auto next_pow2(uint64_t n)
{
    return n == 1UL ? 1UL : 1UL << (64 - __builtin_clzl(n - 1UL));
}
template<typename Type>
constexpr inline Type pow(Type base, Type exp)
{
    int result = 1;
    for (; exp > 0; exp--)
        result *= base;
    return result;
}

template<typename Type>
constexpr inline Type abs(Type num)
{
    return num < 0 ? -num : num;
}

template<typename Type>
constexpr inline Type sign(Type num)
{
    return num > 0 ? 1 : (num < 0 ? -1 : 0);
}

constexpr inline uint64_t jdn(uint8_t days, uint8_t months, uint16_t years)
{
    return (1461 * (years + 4800 + (months - 14) / 12)) / 4 + (367 * (months - 2 - 12 * ((months - 14) / 12))) / 12 - (3 * ((years + 4900 + (months - 14) / 12) / 100)) / 4 + days - 32075;
}
constexpr inline uint64_t jdn_1970 = jdn(1, 1, 1970);

constexpr inline uint64_t epoch(uint64_t seconds, uint64_t minutes, uint64_t hours, uint64_t days, uint64_t months, uint64_t years, uint64_t centuries)
{
    uint64_t jdn_current = jdn(days, months, centuries * 100 + years);
    uint64_t diff = jdn_current - jdn_1970;

    return (diff * (60 * 60 * 24)) + hours * 3600 + minutes * 60 + seconds;
}