// Copyright (C) 2022  ilobilo

#if defined(__x86_64__) || defined(_M_X64)
#include <arch/x86_64/timers/lapic/lapic.hpp>
#include <arch/x86_64/timers/hpet/hpet.hpp>
#include <arch/x86_64/timers/pit/pit.hpp>
#include <arch/x86_64/timers/rtc/rtc.hpp>
#elif defined(__aarch64__) || defined(_M_ARM64)
#endif
#include <lib/timer.hpp>
#include <lib/log.hpp>

namespace timer
{
    void sleep(uint64_t sec)
    {
        #if defined(__x86_64__) || defined(_M_X64)
        if (arch::x86_64::timers::hpet::initialised) arch::x86_64::timers::hpet::sleep(sec);
        else if (arch::x86_64::timers::lapic::initialised) arch::x86_64::timers::lapic::sleep(sec);
        else arch::x86_64::timers::pit::sleep(sec);
        #endif
    }

    void msleep(uint64_t msec)
    {
        #if defined(__x86_64__) || defined(_M_X64)
        if (arch::x86_64::timers::hpet::initialised) arch::x86_64::timers::hpet::msleep(msec);
        else if (arch::x86_64::timers::lapic::initialised) arch::x86_64::timers::lapic::msleep(msec);
        else arch::x86_64::timers::pit::msleep(msec);
        #endif
    }

    void usleep(uint64_t us)
    {
        #if defined(__x86_64__) || defined(_M_X64)
        if (arch::x86_64::timers::hpet::initialised) arch::x86_64::timers::hpet::usleep(us);
        else if (arch::x86_64::timers::lapic::initialised)
        {
            log::warn("HPET has not been initialised! Using LAPIC Timer!");
            arch::x86_64::timers::lapic::msleep(MICS2MS(us));
        }
        else
        {
            log::warn("Neither HPET nor LAPIC Timer has been initialised! Using PIT!");
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
        #if defined(__x86_64__) || defined(_M_X64)
        if (arch::x86_64::timers::hpet::initialised) return arch::x86_64::timers::hpet::counter() / 100000000;
        #endif
        return 0;
    }
}