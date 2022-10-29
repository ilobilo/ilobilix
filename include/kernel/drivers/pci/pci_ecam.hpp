// Copyright (C) 2022  ilobilo

#pragma once

#include <drivers/pci/pci.hpp>

namespace pci::ecam
{
    struct configio final : pci::configio
    {
        private:
        uintptr_t base;
        uint16_t seg;
        uint8_t bus_start;
        uint8_t bus_end;

        std::unordered_map<uintptr_t, uintptr_t> mappings;

        uintptr_t getaddr(uint32_t bus, uint32_t dev, uint32_t func, size_t offset);

        protected:
        uint32_t read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width);
        void write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width);

        public:
        configio(uintptr_t base, uint16_t seg, uint8_t bus_start, uint8_t bus_end) : base(base), seg(seg), bus_start(bus_start), bus_end(bus_end) { }
    };
} // namespace pci::ecam