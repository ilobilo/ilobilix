// Copyright (C) 2022-2024  ilobilo

#include <arch/x86_64/cpu/idt.hpp>
#include <arch/x86_64/lib/io.hpp>
#include <lib/time.hpp>
#include <lib/log.hpp>
#include <atomic>

namespace timers::pit
{
    static constexpr uint64_t frequency = 1000;
    static std::atomic<uint64_t> tick = 0;
    uint8_t vector = 0;

    uint64_t time_ms()
    {
        return tick * (1000 / frequency);
    }

    void msleep(uint64_t ms)
    {
        volatile uint64_t target = time_ms() + ms;
        while (time_ms() < target)
            asm volatile ("pause");
    }

    void init()
    {
        log::infoln("PIT: Initialising...");

        uint64_t divisor = 1193180 / frequency;

        io::out<uint8_t>(0x43, 0x36);

        uint8_t l = static_cast<uint8_t>(divisor);
        uint8_t h = static_cast<uint8_t>(divisor >> 8);

        io::out<uint8_t>(0x40, l);
        io::out<uint8_t>(0x40, h);

        auto [handler, _vector] = idt::allocate_handler(idt::IRQ(0));
        handler.set([](cpu::registers_t *regs)
        {
            tick++;
            time::timer_handler(1'000'000'000 / frequency);
        });
        idt::unmask((vector = _vector) - 0x20);
    }
} // namespace timers::pit