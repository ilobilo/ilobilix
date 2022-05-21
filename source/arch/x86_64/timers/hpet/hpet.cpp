// Copyright (C) 2022  ilobilo

#if defined(__x86_64__)

#include <arch/x86_64/timers/hpet/hpet.hpp>
#include <drivers/acpi/acpi.hpp>
#include <lib/timer.hpp>
#include <lib/mmio.hpp>
#include <lib/log.hpp>

namespace arch::x86_64::timers::hpet
{
    bool initialised = false;
    static uint32_t clk = 0;
    HPET *hpet = nullptr;

    uint64_t counter()
    {
        return mmin<uint64_t>(reinterpret_cast<void*>(&hpet->main_counter_value));
    }

    void usleep(uint64_t us)
    {
        uint64_t target = counter() + (us * 1000000000) / clk;
        while (counter() < target);
    }

    void msleep(uint64_t msec)
    {
        usleep(MS2MICS(msec));
    }

    void sleep(uint64_t sec)
    {
        usleep(SEC2MICS(sec));
    }

    void init()
    {
        if (acpi::hpethdr == nullptr)
        {
            log::error("HPET table not found!\n");
            return;
        }

        hpet = reinterpret_cast<HPET*>(acpi::hpethdr->address.Address);
        clk = hpet->general_capabilities >> 32;

        mmout<uint64_t>(&hpet->general_configuration, 0);
        mmout<uint64_t>(&hpet->main_counter_value, 0);
        mmout<uint64_t>(&hpet->general_configuration, 1);

        initialised = true;
    }
} // namespace arch::x86_64::timers::hpet

#endif