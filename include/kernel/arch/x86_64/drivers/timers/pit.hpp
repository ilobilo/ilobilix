// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <cstdint>

namespace timers::pit
{
    extern uint8_t vector;

    uint64_t time_ms();
    void msleep(uint64_t ms);

    void init();
} // namespace timers::pit