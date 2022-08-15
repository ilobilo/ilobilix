// Copyright (C) 2022  ilobilo

#include <arch/arch.hpp>
#include <lib/time.hpp>
#include <lib/misc.hpp>
#include <lai/host.h>

namespace time
{
    timespec realtime;
    timespec monotonic;

    void timer_handler()
    {
        if (realtime.to_ns() == 0)
            realtime = timespec(arch::epoch(), 0);

        if (monotonic.to_ns() == 0)
            monotonic = timespec(arch::epoch(), 0);

        timespec interval(0, 1'000'000'000 / frequency);
        realtime += interval;
        monotonic += interval;
    }

    uint64_t time_ns()
    {
        return arch::time_ns().value_or(monotonic.to_ns());
    }

    uint64_t time_ms()
    {
        return time_ns() / 1'000'000;
    }

    void nsleep(uint64_t ns)
    {
        uint64_t target = time_ns() + ns;
        while (time_ns() < target)
            arch::pause();
    }

    void msleep(uint64_t ms)
    {
        uint64_t target = time_ms() + ms;
        while (time_ms() < target)
            arch::pause();
    }
} // namespace time

void laihost_sleep(uint64_t ms)
{
    time::msleep(ms);
}

uint64_t laihost_timer()
{
    return time::time_ns() / 100;
}