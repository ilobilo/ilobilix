// Copyright (C) 2022-2024  ilobilo

#include <arch/x86_64/drivers/pci/pci_legacy.hpp>
#include <arch/x86_64/lib/io.hpp>
#include <lib/panic.hpp>

namespace pci::legacy
{
    uint32_t configio::read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width)
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
                PANIC("PCI: Invalid integer size {}", width);
        }
    }

    void configio::write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width)
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
                PANIC("PCI: Invalid integer size {}", width);
        }
    }

    void init_ios()
    {
        auto io = new legacy::configio;
        for (size_t i = 0; i < 256; i++)
            addconfigio(0, i, io);
    }

    void init_rbs()
    {
        auto io = getconfigio(0, 0);
        if (io->read<uint8_t>(0, 0, 0, 0, PCI_HEADER_TYPE) & (1 << 7))
        {
            for (size_t i = 0; i < 8; i++)
            {
                if (io->read<uint16_t>(0, 0, 0, i, PCI_VENDOR_ID) == 0xFFFF)
                    continue;

                addrootbus(new bus_t(nullptr, nullptr, getconfigio(0, i), 0, i));
            }
        }
        else addrootbus(new bus_t(nullptr, nullptr, io, 0, 0));
    }
} // namespace pci::legacy