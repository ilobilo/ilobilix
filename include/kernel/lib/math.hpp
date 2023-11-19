// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <cstdint>
#include <concepts>

extern uintptr_t hhdm_offset;

template<typename Type>
using _get_ret_t = std::conditional_t<std::integral<Type>, std::conditional_t<std::unsigned_integral<Type>, uintptr_t, intptr_t>, Type>;

inline constexpr bool ishh(auto a)
{
    return uintptr_t(a) >= hhdm_offset;
}

template<typename Type, typename Ret = _get_ret_t<Type>>
inline constexpr Ret tohh(Type a)
{
    return ishh(a) ? Ret(a) : Ret(uintptr_t(a) + hhdm_offset);
}

template<typename Type, typename Ret = _get_ret_t<Type>>
inline constexpr Ret fromhh(Type a)
{
    return !ishh(a) ? Ret(a) : Ret(uintptr_t(a) - hhdm_offset);
}

inline constexpr auto align_down(std::integral auto n, std::integral auto a)
{
    return (n & ~(a - 1));
}

inline constexpr auto align_up(std::integral auto n, std::integral auto a)
{
    return align_down(n + a - 1, a);
}

inline constexpr auto div_roundup(std::integral auto n, std::integral auto a)
{
    return align_down(n + a - 1, a) / a;
}

inline constexpr auto next_pow2(uint64_t n)
{
    return n == 1UL ? 1UL : 1UL << (64 - __builtin_clzl(n - 1UL));
}
inline constexpr auto pow(std::integral auto base, std::integral auto exp)
{
    int result = 1;
    for (; exp > 0; exp--)
        result *= base;
    return result;
}

inline constexpr auto abs(std::signed_integral auto num)
{
    return num < 0 ? -num : num;
}

inline constexpr auto sign(std::signed_integral auto num)
{
    return num > 0 ? 1 : (num < 0 ? -1 : 0);
}

inline constexpr uint64_t jdn(uint8_t days, uint8_t months, uint16_t years)
{
    return (1461 * (years + 4800 + (months - 14) / 12)) / 4 + (367 * (months - 2 - 12 * ((months - 14) / 12))) / 12 - (3 * ((years + 4900 + (months - 14) / 12) / 100)) / 4 + days - 32075;
}
inline constexpr uint64_t jdn_1970 = jdn(1, 1, 1970);

inline constexpr uint64_t epoch(uint64_t seconds, uint64_t minutes, uint64_t hours, uint64_t days, uint64_t months, uint64_t years, uint64_t centuries)
{
    uint64_t jdn_current = jdn(days, months, centuries * 100 + years);
    uint64_t diff = jdn_current - jdn_1970;

    return (diff * (60 * 60 * 24)) + hours * 3600 + minutes * 60 + seconds;
}