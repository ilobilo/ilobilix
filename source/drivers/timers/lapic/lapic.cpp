// Copyright (C) 2022  ilobilo

#include <drivers/timers/lapic/lapic.hpp>
#include <cpu/apic/apic.hpp>
#include <cpu/smp/smp.hpp>
#include <lib/timer.hpp>
#include <lib/log.hpp>

namespace timers::lapic
{
    static void mask(bool masked)
    {
        if (masked) apic::lapic::write(0x320, apic::lapic::read(0x320) | (1 << 0x10));
        else apic::lapic::write(0x320, apic::lapic::read(0x320) & ~(1 << 0x10));
    }

    void oneshot(uint8_t vector, uint64_t ms)
    {
        cpu_init();
        mask(true);
        apic::lapic::write(0x3E0, 0x03);
        apic::lapic::write(0x320, (((apic::lapic::read(0x320) & ~(0x03 << 17)) | (0x00 << 17)) & 0xFFFFFF00) | vector);
        apic::lapic::write(0x380, this_cpu->lapic_ticks_in_1ms * ms);
        mask(false);
    }

    void periodic(uint8_t vector, uint64_t ms)
    {
        cpu_init();
        mask(true);
        apic::lapic::write(0x3E0, 0x03);
        apic::lapic::write(0x320, (((apic::lapic::read(0x320) & ~(0x03 << 17)) | (0x01 << 17)) & 0xFFFFFF00) | vector);
        apic::lapic::write(0x380, this_cpu->lapic_ticks_in_1ms * ms);
        mask(false);
    }

    void cpu_init()
    {
        if (apic::initialised == false)
        {
            log::error("APIC has not been initialised!\n");
            return;
        }

        if (this_cpu->lapic_ticks_in_1ms == 0)
        {
            apic::lapic::write(0x3E0, 0x03);
            apic::lapic::write(0x380, 0xFFFFFFFF);
            mask(false);
            timer::msleep(1);
            mask(true);
            this_cpu->lapic_ticks_in_1ms = 0xFFFFFFFF - apic::lapic::read(0x390);
        }
    }

    void init()
    {
        log::info("Initialising LAPIC Timer...");
        cpu_init();
    }
} // namespace timers::lapic