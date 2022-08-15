// Copyright (C) 2022  ilobilo

#pragma once

#include <concepts>
#include <cstdint>

enum class endian
{
    big = __ORDER_BIG_ENDIAN__,
    little = __ORDER_LITTLE_ENDIAN__,
    native = __BYTE_ORDER__
};

template<std::integral Type>
inline constexpr Type bswap(Type num)
{
    if constexpr (sizeof(Type) == sizeof(uint8_t))
        return num;
    else if constexpr (sizeof(Type) == sizeof(uint16_t))
        return __builtin_bswap16(num);
    else if constexpr (sizeof(Type) == sizeof(uint32_t))
        return __builtin_bswap32(num);
    else if constexpr (sizeof(Type) == sizeof(uint64_t))
        return __builtin_bswap64(num);
}

template<endian New, endian Old = endian::native, std::integral Type>
inline constexpr Type convert_endian(Type num)
{
    if constexpr (New == Old)
        return num;

    return bswap(num);
}

template<endian New, std::integral Type>
inline constexpr Type to_endian(Type num)
{
    return convert_endian<New, endian::native>(num);
}

template<endian Old, std::integral Type>
inline constexpr Type from_endian(Type num)
{
    return convert_endian<endian::native, Old>(num);
}