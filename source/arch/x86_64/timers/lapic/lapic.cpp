// Copyright (C) 2022  ilobilo

#if defined(__x86_64__) || defined(_M_X64)

#include <arch/x86_64/timers/lapic/lapic.hpp>
#include <arch/x86_64/apic/apic.hpp>
#include <arch/x86_64/idt/idt.hpp>
#include <lai/helpers/sci.h>
#include <lib/panic.hpp>
#include <lib/timer.hpp>
#include <lib/mmio.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <main.hpp>
#include <cpuid.h>

namespace arch::x86_64::timers::lapic
{
    // TODO: Different per CPU
    static std::atomic<uint64_t> tick = 0;
    uint64_t frequency = default_freq;
    bool initialised = false;

    // TODO: Different per CPU
    static uint64_t ticks_in_1ms = 0;
    // TODO: Different per CPU
    static lock_t lock;

    static void mask(bool masked)
    {
        if (masked) apic::lapic::write(0x320, apic::lapic::read(0x320) | (1 << 0x10));
        else apic::lapic::write(0x320, apic::lapic::read(0x320) & ~(1 << 0x10));
    }

    void initcpu()
    {
        // TODO: Different per CPU
        if (ticks_in_1ms == 0 && apic::initialised == true)
        {
            apic::lapic::write(0x3E0, 0x03);
            apic::lapic::write(0x380, 0xFFFFFFFF);
            mask(false);
            timer::msleep(1);
            mask(true);
            // TODO: Different per CPU
            ticks_in_1ms = 0xFFFFFFFF - apic::lapic::read(0x390);
        }
    }

    void oneshot(uint8_t vector, uint64_t ms)
    {
        lockit(lock);

        initcpu();
        mask(true);
        apic::lapic::write(0x3E0, 0x03);
        apic::lapic::write(0x320, (((apic::lapic::read(0x320) & ~(0x03 << 17)) | (0x00 << 17)) & 0xFFFFFF00) | vector);
        apic::lapic::write(0x380, ticks_in_1ms * ms);
        mask(false);
    }

    void periodic(uint8_t vector, uint64_t ms)
    {
        lockit(lock);

        initcpu();
        mask(true);
        apic::lapic::write(0x3E0, 0x03);
        apic::lapic::write(0x320, (((apic::lapic::read(0x320) & ~(0x03 << 17)) | (0x01 << 17)) & 0xFFFFFF00) | vector);
        apic::lapic::write(0x380, ticks_in_1ms * ms);
        mask(false);
    }

    void sleep(uint64_t sec)
    {
        // TODO: Different per CPU
        if (initialised == false) return;
        uint64_t start = tick;
        while (tick < start + sec * frequency);
    }

    void msleep(uint64_t msec)
    {
        // TODO: Different per CPU
        if (initialised == false) return;
        uint64_t start = tick;
        while (tick < start + msec * (frequency / 100));
    }

    uint64_t get_tick()
    {
        // TODO: Different per CPU
        return tick;
    }

    static void irq_handler(cpu::registers_t *regs)
    {
        // TODO: Different per CPU
        tick++;
    }

    void init()
    {
        if (apic::initialised == false)
        {
            log::error("APIC has not been initialised!\n");
            return;
        }

        initcpu();

        idt::handlers[48].clear();
        idt::handlers[48].set(irq_handler);
        idt::handlers[48].ioapic_redirect(48);

        periodic(48, FREQ2MS(frequency));

        initialised = true;
    }
} // namespace arch::x86_64::timers::lapic

#endif