// Copyright (C) 2022-2024  ilobilo

#include <drivers/pci/pci_ecam.hpp>
#include <lib/panic.hpp>
#include <lib/mmio.hpp>
#include <mm/vmm.hpp>

namespace pci::ecam
{
    static uintptr_t base_addr = -1;
    uintptr_t configio::getaddr(uint32_t bus, uint32_t dev, uint32_t func, size_t offset)
    {
        assert(bus >= this->bus_start && bus <= this->bus_end);
        uintptr_t phys_addr = (this->base + ((bus - this->bus_start) << 20) | (dev << 15) | (func << 12));

        if (this->mappings.contains(phys_addr))
            return this->mappings[phys_addr] + offset;

        if (base_addr == static_cast<uintptr_t>(-1))
            base_addr = vmm::alloc_vspace(vmm::vsptypes::ecam);

        static constexpr uintptr_t size = 1 << 20;

        auto virt_addr = base_addr;
        base_addr += size;

        vmm::kernel_pagemap->map_range(virt_addr, phys_addr, size, vmm::rw, vmm::mmio);
        this->mappings[phys_addr] = virt_addr;

        return virt_addr + offset;
    }

    uint32_t configio::read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width)
    {
        assert(seg == this->seg);
        auto addr = this->getaddr(bus, dev, func, offset);
        switch (width)
        {
            case sizeof(uint8_t):
                return mmio::in<uint8_t>(addr);
                break;
            case sizeof(uint16_t):
                return mmio::in<uint16_t>(addr);
                break;
            case sizeof(uint32_t):
                return mmio::in<uint32_t>(addr);
                break;
            default:
                PANIC("PCI: Invalid integer size {}", width);
        }
    }

    void configio::write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width)
    {
        assert(seg == this->seg);
        auto addr = this->getaddr(bus, dev, func, offset);
        switch (width)
        {
            case sizeof(uint8_t):
                mmio::out<uint8_t>(addr, value);
                break;
            case sizeof(uint16_t):
                mmio::out<uint16_t>(addr, value);
                break;
            case sizeof(uint32_t):
                mmio::out<uint32_t>(addr, value);
                break;
            default:
                PANIC("PCI: Invalid integer size {}", width);
        }
    }
} // namespace pci::ecam