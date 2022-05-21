// Copyright (C) 2022  ilobilo

#if defined(__x86_64__)

#include <arch/x86_64/misc/io.hpp>
#include <arch/x86_64/pci/pci.hpp>
#include <lib/log.hpp>

namespace arch::x86_64::pci
{
    uint32_t get_addr(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset)
    {
        uint32_t address = (bus << 16) | (dev << 11) | (func << 8) | (offset & ~(3)) | 0x80000000;
        return address;
    }

    uint32_t read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width)
    {
        outl(0xCF8, get_addr(seg, bus, dev, func, offset));
        if (width == 1) return inb(0xCFC + (offset & 3));
        else if (width == 2) return inw(0xCFC + (offset & 3));
        else if (width == 4) return inl(0xCFC + (offset & 3));
        return 0;
    }

    void write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width)
    {
        outl(0xCF8, get_addr(seg, bus, dev, func, offset));
        if (width == 1) outb(0xCFC + (offset & 3), value);
        else if (width == 2) outw(0xCFC + (offset & 3), value);
        else if (width == 4) outl(0xCFC + (offset & 3), value);
    }
} // namespace arch::x86_64::pci

#endif