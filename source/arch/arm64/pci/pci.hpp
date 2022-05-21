// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__aarch64__)

#include <cstddef>
#include <cstdint>

namespace arch::arm64::pci
{
    uint32_t read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width = 8);
    void write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width = 8);
} // namespace arch::arm64::pci

#endif