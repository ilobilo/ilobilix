// Copyright (C) 2022  ilobilo

#include <drivers/pci/pci_ecam.hpp>
#include <lib/panic.hpp>
#include <lib/misc.hpp>
#include <lib/mmio.hpp>
#include <lib/log.hpp>
#include <mm/vmm.hpp>

namespace pci
{
    uintptr_t ecam_configio::getaddr(uint32_t bus, uint32_t dev, uint32_t func, size_t offset)
    {
        assert(bus >= this->bus_start && bus <= this->bus_end);
        uintptr_t phys_addr = (this->base + ((bus - this->bus_start) << 20) | (dev << 15) | (func << 12));

        // Figure out why this dowsn't work on vmware and real hw (gpf)
        // if (this->mappings.contains(phys_addr))
        //     return this->mappings[phys_addr] + offset;

        // uintptr_t virt_addr = tohh(phys_addr);
        // mm::vmm::kernel_pagemap->mapMe(mvirt_addr, phys_addr, mm::vmm::RW, mm::vmm::MMIO);
        // this->mappings[phys_addr] = virt_addr;

        // return virt_addr + offset;

        return phys_addr;
    }

    uint32_t ecam_configio::read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width)
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
                PANIC("PCI: Invalid integer size!");
        }
    }

    void ecam_configio::write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width)
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
                PANIC("PCI: Invalid integer size!");
        }
    }
} // namespace pci