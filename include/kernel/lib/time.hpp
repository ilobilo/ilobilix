// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <lib/types.hpp>
#include <lib/event.hpp>
#include <cstdint>

namespace time
{
    struct timer
    {
        timespec when;
        event_t event;
        bool armed;
        bool fired;

        timer(timespec when);
        ~timer();
    };

    extern timespec realtime;
    extern timespec monotonic;

    void init();

    void timer_handler(size_t ns);

    uint64_t time_ns();
    uint64_t time_ms();

    void msleep(uint64_t ms);
    void nsleep(uint64_t ns);
} // namespace time