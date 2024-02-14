// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <drivers/pci/pci.hpp>

namespace pci::legacy
{
    struct configio final : pci::configio
    {
        protected:
        uint32_t read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width);
        void write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width);
    };

    void init_ios();
    void init_rbs();
} // namespace pci::legacy