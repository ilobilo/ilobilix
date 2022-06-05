// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

namespace timers::rtc
{
    uint8_t century(), year(), month(), day(), hour(), minute(), second();
    uint8_t time();
    uint8_t epoch();
    uint8_t seconds_since_boot();

    void sleep(uint64_t sec);
} // namespace timers::rtc