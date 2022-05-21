// Copyright (C) 2022  ilobilo

#if defined(__aarch64__)

#include <arch/arm64/pci/pci.hpp>
#include <lib/log.hpp>

namespace arch::arm64::pci
{
    uint32_t get_addr(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset)
    {
        return nullptr;
    }

    uint32_t read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width)
    {
        return 0;
    }

    void write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width)
    {
    }
} // namespace arch::arm64::pci

#endif