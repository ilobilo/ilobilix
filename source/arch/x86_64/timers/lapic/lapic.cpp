// Copyright (C) 2022  ilobilo

#if defined(__x86_64__)

#include <arch/x86_64/timers/lapic/lapic.hpp>
#include <arch/x86_64/apic/apic.hpp>
#include <lib/timer.hpp>
#include <lib/log.hpp>
#include <smp/smp.hpp>

namespace arch::x86_64::timers::lapic
{
    static void mask(bool masked)
    {
        if (masked) apic::lapic::write(0x320, apic::lapic::read(0x320) | (1 << 0x10));
        else apic::lapic::write(0x320, apic::lapic::read(0x320) & ~(1 << 0x10));
    }

    void oneshot(uint8_t vector, uint64_t ms)
    {
        init();
        mask(true);
        apic::lapic::write(0x3E0, 0x03);
        apic::lapic::write(0x320, (((apic::lapic::read(0x320) & ~(0x03 << 17)) | (0x00 << 17)) & 0xFFFFFF00) | vector);
        apic::lapic::write(0x380, this_cpu->lapic_ticks_in_1ms * ms);
        mask(false);
    }

    void periodic(uint8_t vector, uint64_t ms)
    {
        init();
        mask(true);
        apic::lapic::write(0x3E0, 0x03);
        apic::lapic::write(0x320, (((apic::lapic::read(0x320) & ~(0x03 << 17)) | (0x01 << 17)) & 0xFFFFFF00) | vector);
        apic::lapic::write(0x380, this_cpu->lapic_ticks_in_1ms * ms);
        mask(false);
    }

    void init()
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
} // namespace arch::x86_64::timers::lapic

#endif