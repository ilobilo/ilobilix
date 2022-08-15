// Copyright (C) 2022  ilobilo

#include <arch/x86_64/lib/io.hpp>
#include <lai/host.h>

void laihost_outb(uint16_t port, uint8_t val)
{
    io::out<uint8_t>(port, val);
}

void laihost_outw(uint16_t port, uint16_t val)
{
    io::out<uint16_t>(port, val);
}

void laihost_outd(uint16_t port, uint32_t val)
{
    io::out<uint32_t>(port, val);
}

uint8_t laihost_inb(uint16_t port)
{
    return io::in<uint8_t>(port);
}

uint16_t laihost_inw(uint16_t port)
{
    return io::in<uint16_t>(port);
}

uint32_t laihost_ind(uint16_t port)
{
    return io::in<uint32_t>(port);
}