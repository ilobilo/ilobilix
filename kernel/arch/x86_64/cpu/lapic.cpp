// Copyright (C) 2022-2023  ilobilo

#include <arch/x86_64/cpu/lapic.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/lib/io.hpp>
#include <drivers/acpi.hpp>
#include <lib/time.hpp>
#include <lib/mmio.hpp>
#include <lib/misc.hpp>
#include <mm/vmm.hpp>

namespace lapic
{
    bool lapic::x2apic_check()
    {
        uint32_t a, b, c, d;
        if (cpu::id(1, 0, a, b, c, d) == false)
            return this->x2apic = false;

        if (!(c & (1 << 21)))
            return this->x2apic = false;

        auto dmar = acpi::findtable<acpi::DMARHeader>("DMAR", 0);

        if (dmar == nullptr)
            return this->x2apic = true;

        if ((dmar->flags & (1 << 0)) && (dmar->flags & (1 << 1)))
            return this->x2apic = false;

        return this->x2apic = true;
    }

    uint32_t lapic::read(uint32_t reg)
    {
        if (this->x2apic == true)
            return cpu::rdmsr(reg2x2apic(reg));
        return mmio::in<uint32_t>(this->mmio_base + reg);
    }

    void lapic::write(uint32_t reg, uint64_t value)
    {
        if (this->x2apic == true)
            cpu::wrmsr(reg2x2apic(reg), value);
        mmio::out<uint32_t>(this->mmio_base + reg, value);
    }

    void lapic::timer_calibrate()
    {
        this->write(0x3E0, 0x03);
        this->write(0x380, 0xFFFFFFFF);

        this->write(0x320, this->read(0x320) & ~(1 << 16));
        time::msleep(10);
        this->write(0x320, this->read(0x320) | (1 << 16));

        this->ticks_per_ms = (0xFFFFFFFF - this->read(0x390)) / 10;
    }

    void lapic::init()
    {
        x2apic_check();

        auto base = cpu::rdmsr(0x1B) | (1 << 11);
        if (this->x2apic == true)
            base |= (1 << 10);
        cpu::wrmsr(0x1B, base);

        auto phys_mmio_base = base & ~(0xFFF);

        this->mmio_base = tohh(phys_mmio_base);
        // this->mmio_base = malloc<uintptr_t>(0x1000);

        // if (this->x2apic == false)
        //     vmm::kernel_pagemap->map(this->mmio_base, phys_mmio_base, vmm::rw);

        this->id = (this->x2apic ? this->read(0x20) : (this->read(0x20) >> 24) & 0xFF);

        this->write(0x80, 0x00);
        this->write(0xF0, 0xFF | (1 << 8));
    }

    void lapic::ipi(uint32_t flags, uint32_t id)
    {
        if (this->x2apic == true)
            this->write(0x300, (static_cast<uint64_t>(id) << 32) | (1 << 14) | flags);
        else
        {
            this->write(0x310, id << 24);
            this->write(0x300, flags);
        }
    }

    void lapic::eoi()
    {
        this->write(0xB0, 0x00);
    }

    void lapic::timer(uint8_t vector, uint64_t ms, timerModes mode)
    {
        if (this->ticks_per_ms == 0)
            this->timer_calibrate();

        uint64_t ticks = this->ticks_per_ms * ms;

        this->write(0x3E0, 0x03);
        uint64_t value = this->read(0x320) & ~(3 << 17);

        value |= std::to_underlying(mode) << 17;
        value &= 0xFFFFFF00;
        value |= vector;

        this->write(0x320, value);
        this->write(0x380, ticks ? ticks : 1);
        this->write(0x320, this->read(0x320) & ~(1 << 16));
    }
} // namespace lapic