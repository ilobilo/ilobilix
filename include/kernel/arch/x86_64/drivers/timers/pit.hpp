// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

namespace timers::pit
{
    uint64_t time_ms();
    void msleep(uint64_t ms);

    void init();
} // namespace timers::pit