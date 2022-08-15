// Copyright (C) 2022  ilobilo

#pragma once

#include <type_traits>
#include <cstdint>

namespace io
{
    template<typename Type> requires (std::is_same_v<Type, uint8_t> || std::is_same_v<Type, uint16_t> || std::is_same_v<Type, uint32_t>)
    static inline Type in(uint16_t port)
    {
        Type data = Type(0);
        if constexpr (std::is_same_v<Type, uint8_t>) asm volatile("inb %w1, %b0" : "=a" (data) : "Nd" (port));
        else if constexpr (std::is_same_v<Type, uint16_t>) asm volatile("inw %w1, %w0" : "=a" (data) : "Nd" (port));
        else if constexpr (std::is_same_v<Type, uint32_t>) asm volatile("inl %w1, %0" : "=a" (data) : "Nd" (port));
        return data;
    }

    template<typename Type> requires (std::is_same_v<Type, uint8_t> || std::is_same_v<Type, uint16_t> || std::is_same_v<Type, uint32_t>)
    static inline void out(uint16_t port, Type val)
    {
        if constexpr (std::is_same_v<Type, uint8_t>) asm volatile ("outb %b0, %w1" :: "a"(val), "Nd"(port));
        else if constexpr (std::is_same_v<Type, uint16_t>) asm volatile("outw %w0, %w1" :: "a" (val), "Nd" (port));
        else if constexpr (std::is_same_v<Type, uint32_t>) asm volatile("outl %0, %w1" :: "a" (val), "Nd" (port));
    }
} // namespace io