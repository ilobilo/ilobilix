// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__x86_64__)

#include <cstddef>
#include <cstdint>

namespace arch::x86_64::timers::lapic
{
    void oneshot(uint8_t vector, uint64_t ms = 1);
    void periodic(uint8_t vector, uint64_t ms = 1);

    void init();
} // namespace arch::x86_64::timers::lapic

#endif