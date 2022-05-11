// Copyright (C) 2022  ilobilo

#if defined(__x86_64__) || defined(_M_X64)

#include <arch/x86_64/misc/io.hpp>
#include <drivers/acpi/acpi.hpp>

namespace arch::x86_64::timers::rtc
{
    uint8_t bcdtobin(uint8_t value)
    {
        return (value >> 4) * 10 + (value & 15);
    }

    uint8_t read(uint8_t write)
    {
        outb(0x70, write);
        return bcdtobin(inb(0x71));
    }

    uint8_t century()
    {
        if (acpi::fadthdr && acpi::fadthdr->Century == 0) return 20;
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
        while (lastsec == time());

        lastsec = time() + sec;
        while (lastsec != time());
    }
} // namespace arch::x86_64::timers::rtc

#endif