// Copyright (C) 2022  ilobilo

#include <lib/mmio.hpp>

void mmoutb(void *addr, uint8_t value)
{
    volatile auto ptr = reinterpret_cast<volatile uint8_t*>(addr);
    *ptr = value;
}

void mmoutw(void *addr, uint16_t value)
{
    volatile auto ptr = reinterpret_cast<volatile uint16_t*>(addr);
    *ptr = value;
}

void mmoutl(void *addr, uint32_t value)
{
    volatile auto ptr = reinterpret_cast<volatile uint32_t*>(addr);
    *ptr = value;
}

void mmoutq(void *addr, uint64_t value)
{
    volatile auto ptr = reinterpret_cast<volatile uint64_t*>(addr);
    *ptr = value;
}

uint8_t mminb(void *addr)
{
    volatile auto ptr = reinterpret_cast<volatile uint8_t*>(addr);
    return *ptr;
}

uint16_t mminw(void *addr)
{
    volatile auto ptr = reinterpret_cast<volatile uint16_t*>(addr);
    return *ptr;
}

uint32_t mminl(void *addr)
{
    volatile auto ptr = reinterpret_cast<volatile uint32_t*>(addr);
    return *ptr;
}

uint64_t mminq(void *addr)
{
    volatile auto ptr = reinterpret_cast<volatile uint64_t*>(addr);
    return *ptr;
}