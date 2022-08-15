// Copyright (C) 2022  ilobilo

#include <arch/x86_64/drivers/timers/tsc.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <drivers/smp.hpp>
#include <lib/time.hpp>
#include <lib/log.hpp>

namespace timers::tsc
{
    bool initialised = false;

    uint64_t rdtsc()
    {
        uint32_t a, d;
        asm volatile (
            "lfence \n\t"
            "rdtsc"
            : "=a"(a), "=d"(d)
            :: "memory");

        return a | (static_cast<uint64_t>(d) << 32);
    }

    uint64_t time_ns()
    {
        return rdtsc() / this_cpu()->tsc_ticks_per_ns;
    }

    void nsleep(uint64_t ns)
    {
        volatile uint64_t target = time_ns() + ns;
        while (time_ns() < target)
            asm volatile ("pause");
    }

    void init()
    {
        uint32_t a, b, c, d;
        bool ret = cpu::id(0x80000007, 0, a, b, c, d);

        if (ret == false || !(d & (1 << 8)))
        {
            log::error("Invariant TSC not supported!");
            return;
        }

        auto start = rdtsc();
        time::msleep(10);
        auto end = rdtsc();

        this_cpu()->tsc_ticks_per_ns = (end - start) / 10;

        tsc::initialised = true;
    }
} // namespace timers::tsc