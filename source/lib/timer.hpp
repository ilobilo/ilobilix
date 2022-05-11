// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

namespace timer
{
    #define SEC2MICS(num) ((num) * 1000000)
    #define MS2MICS(num) ((num) * 1000)

    #define MICS2SEC(num) ((num) / 1000000)
    #define MICS2MS(num) ((num) / 1000)

    #define MS2FREQ(ms) (100 / (ms))
    #define FREQ2MS(freq) (100 / (freq))

    void sleep(uint64_t sec);
    void msleep(uint64_t msec);
    void usleep(uint64_t us);
}