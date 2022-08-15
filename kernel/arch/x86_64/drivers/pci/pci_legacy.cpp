// Copyright (C) 2022  ilobilo

#include <arch/x86_64/drivers/pci/pci_legacy.hpp>
#include <arch/x86_64/lib/io.hpp>
#include <lib/panic.hpp>

namespace pci
{
    uint32_t legacy_configio::read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width)
    {
        assert(seg == 0);
        uint32_t address = (bus << 16) | (dev << 11) | (func << 8) | (offset & ~(3U)) | 0x80000000;
        io::out<uint32_t>(0xCF8, address);
        switch (width)
        {
            case sizeof(uint8_t):
                return io::in<uint8_t>(0xCFC + (offset & 3));
            case sizeof(uint16_t):
                return io::in<uint16_t>(0xCFC + (offset & 3));
            case sizeof(uint32_t):
                return io::in<uint32_t>(0xCFC + (offset & 3));
            default:
                PANIC("PCI: Invalid integer size!");
        }
    }

    void legacy_configio::write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width)
    {
        assert(seg == 0);
        uint32_t address = (bus << 16) | (dev << 11) | (func << 8) | (offset & ~(3U)) | 0x80000000;
        io::out<uint32_t>(0xCF8, address);
        switch (width)
        {
            case sizeof(uint8_t):
                io::out<uint8_t>(0xCFC + (offset & 3), value);
                break;
            case sizeof(uint16_t):
                io::out<uint16_t>(0xCFC + (offset & 3), value);
                break;
            case sizeof(uint32_t):
                io::out<uint32_t>(0xCFC + (offset & 3), value);
                break;
            default:
                PANIC("PCI: Invalid integer size!");
        }
    }
} // namespace pci