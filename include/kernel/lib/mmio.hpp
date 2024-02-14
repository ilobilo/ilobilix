// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <concepts>

namespace mmio
{
    template<std::unsigned_integral Type>
    static inline Type in(auto addr)
    {
        volatile auto ptr = reinterpret_cast<volatile Type*>(addr);
        return *ptr;
    }

    template<std::unsigned_integral Type>
    static inline void out(auto addr, Type value)
    {
        volatile auto ptr = reinterpret_cast<volatile Type*>(addr);
        *ptr = value;
    }
} // namespace mmio