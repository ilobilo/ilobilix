// Copyright (C) 2022  ilobilo

#pragma once

#include <type_traits>
#include <cstdint>

template<typename type>
concept valuetype = (std::is_same<type, uint8_t>() || std::is_same<type, uint16_t>() || std::is_same<type, uint32_t>() || std::is_same<type, uint64_t>());

template<valuetype type>
static inline type mmin(void *addr)
{
    volatile auto ptr = reinterpret_cast<volatile type*>(addr);
    return *ptr;
}

template<valuetype type>
static inline void mmout(void *addr, type value)
{
    volatile auto ptr = reinterpret_cast<volatile type*>(addr);
    *ptr = value;
}