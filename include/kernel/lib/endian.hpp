// Copyright (C) 2022  ilobilo

#pragma once

#include <concepts>
#include <cstdint>
#include <bit>

template<std::endian New, std::endian Old = std::endian::native, std::integral Type>
inline constexpr Type convert_endian(Type num)
{
    if constexpr (New == Old)
        return num;

    return std::byteswap(num);
}

template<std::endian New, std::integral Type>
inline constexpr Type to_endian(Type num)
{
    return convert_endian<New, std::endian::native>(num);
}

template<std::endian Old, std::integral Type>
inline constexpr Type from_endian(Type num)
{
    return convert_endian<std::endian::native, Old>(num);
}

template<typename Type, std::endian E>
struct endian_storage
{
    using type = Type;
    Type value;

    constexpr endian_storage() = default;
    constexpr endian_storage(Type value) : value(convert_endian<E, std::endian::native>(static_cast<Type>(value))) { }

    Type load()
    {
        return convert_endian<std::endian::native, E>(this->value);
    }

    void store(Type value)
    {
        this->value = convert_endian<E, std::endian::native>(value);
    }
};