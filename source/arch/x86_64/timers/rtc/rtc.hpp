// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__x86_64__)

#include <cstdint>

namespace arch::x86_64::timers::rtc
{
    uint8_t century(), year(), month(), day(), hour(), minute(), second();
    uint8_t time();
    uint8_t epoch();
    uint8_t seconds_since_boot();

    void sleep(uint64_t sec);
} // namespace arch::x86_64::timers::rtc

#endif