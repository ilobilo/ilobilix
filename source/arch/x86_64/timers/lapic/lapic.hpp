// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__x86_64__) || defined(_M_X64)

#include <cstddef>
#include <cstdint>

namespace arch::x86_64::timers::lapic
{
    static constexpr uint8_t default_freq = 100;

    extern uint64_t frequency;
    extern bool initialised;

    void oneshot(uint8_t vector, uint64_t ms = 1);
    void periodic(uint8_t vector, uint64_t ms = 1);

    void sleep(uint64_t sec);
    void msleep(uint64_t msec);
    uint64_t get_tick();

    void init();
} // namespace arch::x86_64::timers::lapic

#endif