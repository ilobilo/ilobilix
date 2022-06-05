// Copyright (C) 2022  ilobilo

#include <drivers/timers/pit/pit.hpp>
#include <cpu/idt/idt.hpp>
#include <lib/timer.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>
#include <atomic>

namespace timers::pit
{
    static std::atomic<uint64_t> tick = 0;
    uint64_t frequency = default_freq;
    static lock_t lock;

    void sleep(uint64_t sec)
    {
        uint64_t start = tick;
        while (tick < start + sec * frequency);
    }

    void msleep(uint64_t msec)
    {
        uint64_t start = tick;
        while (tick < start + msec * (frequency / 100));
    }

    uint64_t get_tick()
    {
        return tick;
    }

    static void irq_handler(cpu::registers_t *regs)
    {
        tick++;
    }

    void setfreq(uint64_t freq)
    {
        lockit(lock);

        if (freq < 19) freq = 19;
        frequency = freq;
        uint64_t divisor = 1193180 / frequency;

        outb(0x43, 0x36);

        uint8_t l = static_cast<uint8_t>(divisor);
        uint8_t h = static_cast<uint8_t>(divisor >> 8);

        outb(0x40, l);
        outb(0x40, h);
    }

    void init()
    {
        log::info("Initialising PIT...");

        setfreq(default_freq);
        idt::handlers[idt::IRQ0].set(irq_handler);
        idt::handlers[idt::IRQ0].ioapic_redirect(idt::IRQ0);
    }
} // namespace timers::pit