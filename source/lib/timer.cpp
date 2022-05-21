// Copyright (C) 2022  ilobilo

#if defined(__x86_64__)
#include <arch/x86_64/timers/hpet/hpet.hpp>
#include <arch/x86_64/timers/pit/pit.hpp>
#elif defined(__aarch64__)
#endif
#include <lib/timer.hpp>
#include <lib/log.hpp>

namespace timer
{
    void sleep(uint64_t sec)
    {
        #if defined(__x86_64__)
        if (arch::x86_64::timers::hpet::initialised) arch::x86_64::timers::hpet::sleep(sec);
        else arch::x86_64::timers::pit::sleep(sec);
        #endif
    }

    void msleep(uint64_t msec)
    {
        #if defined(__x86_64__)
        if (arch::x86_64::timers::hpet::initialised) arch::x86_64::timers::hpet::msleep(msec);
        else arch::x86_64::timers::pit::msleep(msec);
        #endif
    }

    void usleep(uint64_t us)
    {
        #if defined(__x86_64__)
        if (arch::x86_64::timers::hpet::initialised) arch::x86_64::timers::hpet::usleep(us);
        else
        {
            log::warn("HPET has not been initialised! Using PIT!");
            arch::x86_64::timers::pit::msleep(MICS2MS(us));
        }
        #endif
    }


    void laihost_sleep(uint64_t ms)
    {
        msleep(ms);
    }

    uint64_t laihost_timer()
    {
        #if defined(__x86_64__)
        if (arch::x86_64::timers::hpet::initialised) return arch::x86_64::timers::hpet::counter() / 100000000;
        #endif
        return 0;
    }
}