// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/types.hpp>
#include <cstdint>

namespace time
{
    static constexpr uint64_t frequency = 1'000;

    extern timespec realtime;
    extern timespec monotonic;

    void timer_handler();

    uint64_t time_ns();
    uint64_t time_ms();

    void msleep(uint64_t ms);
    void nsleep(uint64_t ns);
} // namespace time