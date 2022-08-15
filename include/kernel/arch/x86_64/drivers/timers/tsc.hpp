// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

namespace timers::tsc
{
    extern bool initialised;

    uint64_t rdtsc();
    uint64_t time_ns();

    void nsleep(uint64_t ns);

    void init();
} // namespace timers::tsc