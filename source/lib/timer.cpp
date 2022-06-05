// Copyright (C) 2022  ilobilo

#include <drivers/timers/hpet/hpet.hpp>
#include <drivers/timers/pit/pit.hpp>
#include <lib/timer.hpp>
#include <lib/log.hpp>
#include <lai/host.h>

namespace timer
{
    void sleep(uint64_t sec)
    {
        if (timers::hpet::initialised) timers::hpet::sleep(sec);
        else timers::pit::sleep(sec);
    }

    void msleep(uint64_t msec)
    {
        if (timers::hpet::initialised) timers::hpet::msleep(msec);
        else timers::pit::msleep(msec);
    }

    void usleep(uint64_t us)
    {
        if (timers::hpet::initialised) timers::hpet::usleep(us);
        else
        {
            log::warn("HPET has not been initialised! Using PIT!");
            timers::pit::msleep(MICS2MS(us));
        }
    }
}

void laihost_sleep(uint64_t ms)
{
    timer::msleep(ms);
}

uint64_t laihost_timer()
{
    if (timers::hpet::initialised == true) return timers::hpet::counter() / 100000000;
    return 0;
}