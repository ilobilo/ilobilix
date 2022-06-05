// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

namespace timers::pit
{
    static constexpr uint8_t default_freq = 100;

    extern uint64_t frequency;

    void msleep(uint64_t msec);
    void sleep(uint64_t sec);

    void setfreq(uint64_t freq = default_freq);
    uint64_t getfreq();
    void resetfreq();

    uint64_t get_tick();

    void init();
} // namespace timers::pit