// Copyright (C) 2022  ilobilo

#pragma once

#include <cstddef>
#include <cstdint>

namespace timers::lapic
{
    void oneshot(uint8_t vector, uint64_t ms = 1);
    void periodic(uint8_t vector, uint64_t ms = 1);

    void cpu_init();
    void init();
} // namespace timers::lapic