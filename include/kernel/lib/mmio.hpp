// Copyright (C) 2022  ilobilo

#pragma once

#include <type_traits>
#include <cstdint>

namespace mmio
{
    template<typename Type> requires (std::is_same_v<Type, uint8_t> || std::is_same_v<Type, uint16_t> || std::is_same_v<Type, uint32_t> || std::is_same_v<Type, uint64_t>)
    static inline Type in(auto addr)
    {
        volatile auto ptr = reinterpret_cast<volatile Type*>(addr);
        return *ptr;
    }

    template<typename Type> requires (std::is_same_v<Type, uint8_t> || std::is_same_v<Type, uint16_t> || std::is_same_v<Type, uint32_t> || std::is_same_v<Type, uint64_t>)
    static inline void out(auto addr, Type value)
    {
        volatile auto ptr = reinterpret_cast<volatile Type*>(addr);
        *ptr = value;
    }
} // namespace mmio