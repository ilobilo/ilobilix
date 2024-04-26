// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <concepts>
#include <utility>
#include <cstdint>

extern uintptr_t hhdm_offset;

template<typename Type, template<typename> typename Conj>
concept wrap_cvref = Conj<std::remove_cvref_t<Type>>::value;

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

template<std::integral Type1, std::integral Type2>
inline constexpr auto align_down(Type1 n, Type2 a)
{
    constexpr auto align_down_internal = [&](auto n, auto a)
    {
        return (n & ~(a - 1));
    };
    return align_down_internal(std::make_unsigned_t<Type1>(n), std::make_unsigned_t<Type2>(a));
}

inline constexpr auto align_up(std::integral auto n, std::integral auto a)
{
    return align_down(n + a - 1, a);
}

inline constexpr auto div_roundup(std::integral auto n, std::integral auto a)
{
    return align_down(n + a - 1, a) / a;
}

template<std::unsigned_integral Type>
inline constexpr auto next_pow2(Type n)
{
    constexpr Type one { 1 };
    return n == one ? one : one << ((sizeof(Type) * 8) - __builtin_clzl(n - one));
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

inline constexpr auto unique_from(wrap_cvref<std::is_integral> auto a)
{
    return a;
}

template<wrap_cvref<std::is_integral> ...Args>
inline constexpr auto unique_from(wrap_cvref<std::is_integral> auto a, Args &&...args)
{
    constexpr auto cantor_pair = [](auto x, auto y)
    {
        return ((x + y) * (x + y + 1)) / 2 + y;
    };

    if constexpr (sizeof...(Args) == 1)
        return cantor_pair(a, std::forward<Args>(args)...);

    return cantor_pair(a, unique_from(std::forward<Args>(args)...));
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