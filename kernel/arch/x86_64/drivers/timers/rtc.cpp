// Copyright (C) 2022-2024  ilobilo

#include <drivers/acpi.hpp>
#include <arch/arch.hpp>

#include <lib/io.hpp>

namespace timers::rtc
{
    uint8_t bcdtobin(uint8_t value)
    {
        return (value >> 4) * 10 + (value & 15);
    }

    uint8_t read(uint8_t write)
    {
        io::out<uint8_t>(0x70, write);
        return bcdtobin(io::in<uint8_t>(0x71));
    }

    uint8_t century()
    {
        auto fadt = acpi::get_fadt();
        if (fadt != nullptr && fadt->century == 0)
            return 20;

        return read(0x32);
    }

    uint8_t year()
    {
        return read(0x09);
    }

    uint8_t month()
    {
        return read(0x08);
    }

    uint8_t day()
    {
        return read(0x07);
    }

    uint8_t hour()
    {
        return read(0x04);
    }

    uint8_t minute()
    {
        return read(0x02);
    }

    uint8_t second()
    {
        return read(0x00);
    }

    uint8_t time()
    {
        return hour() * 3600 + minute() * 60 + second();
    }

    void sleep(uint64_t sec)
    {
        uint64_t lastsec = time();
        while (lastsec == time())
            arch::pause();

        lastsec = time() + sec;
        while (lastsec != time())
            arch::pause();
    }
} // namespace timers::rtc