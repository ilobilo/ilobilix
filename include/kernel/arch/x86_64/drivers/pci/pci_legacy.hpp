// Copyright (C) 2022  ilobilo

#pragma once

#include <drivers/pci/pci.hpp>

namespace pci
{
    struct legacy_configio final : configio
    {
        protected:
        uint32_t read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width);
        void write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width);
    };
} // namespace pci