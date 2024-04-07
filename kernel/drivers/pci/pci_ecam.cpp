// Copyright (C) 2022-2024  ilobilo

#include <drivers/pci/pci_ecam.hpp>
#include <lib/panic.hpp>
#include <lib/mmio.hpp>
#include <mm/vmm.hpp>

namespace pci::ecam
{
    uintptr_t configio::getaddr(uint32_t bus, uint32_t dev, uint32_t func, size_t offset)
    {
        assert(bus >= this->bus_start && bus <= this->bus_end);
        uintptr_t phys_addr = (this->base + ((bus - this->bus_start) << 20) | (dev << 15) | (func << 12));

        if (this->mappings.contains(phys_addr))
            return this->mappings[phys_addr] + offset;

        static constexpr uintptr_t size = 1 << 20;
        auto vaddr = vmm::alloc_vspace(vmm::vsptypes::ecam, size, sizeof(uint32_t));

        vmm::kernel_pagemap->map_range(vaddr, phys_addr, size, vmm::rw, vmm::mmio);
        this->mappings[phys_addr] = vaddr;

        return vaddr + offset;
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