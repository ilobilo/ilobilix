// Copyright (C) 2022  ilobilo

#if defined(__x86_64__)

#include <lai/host.h>
#include <cstdint>

void outb(uint16_t port, uint8_t val)
{
    asm volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}

void outw(uint16_t port, uint16_t val)
{
    asm volatile("outw %w0, %w1" :: "a" (val), "Nd" (port));
}

void outl(uint16_t port, uint32_t val)
{
    asm volatile("outl %0, %w1" :: "a" (val), "Nd" (port));
}

uint8_t inb(uint16_t port)
{
    uint8_t data;
    asm volatile("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

uint16_t inw(uint16_t port)
{
    uint16_t data;
    asm volatile("inw %w1, %w0" : "=a" (data) : "Nd" (port));
    return data;
}

uint32_t inl(uint16_t port)
{
    uint32_t data;
    asm volatile("inl %w1, %0" : "=a" (data) : "Nd" (port));
    return data;
}

void io_wait(void)
{
    asm volatile ("outb %%al, $0x80" :: "a"(0));
}

void laihost_outb(uint16_t port, uint8_t val)
{
    outb(port, val);
}

void laihost_outw(uint16_t port, uint16_t val)
{
    outw(port, val);
}

void laihost_outd(uint16_t port, uint32_t val)
{
    outl(port, val);
}

uint8_t laihost_inb(uint16_t port)
{
    return inb(port);
}

uint16_t laihost_inw(uint16_t port)
{
    return inw(port);
}

uint32_t laihost_ind(uint16_t port)
{
    return inl(port);
}

#endif