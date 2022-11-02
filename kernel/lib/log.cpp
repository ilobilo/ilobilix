// Copyright (C) 2022  ilobilo

#include <drivers/serial.hpp>
#include <drivers/term.hpp>
#include <frg/macros.hpp>
#include <lib/log.hpp>
#include <lai/host.h>

namespace log
{
    void prints(const char *str, size_t length)
    {
        while (length--)
        {
            term::printc(*str);
            serial::printc(*str++);
        }
    }

    void prints(const char *str)
    {
        term::print(str);
        while (*str)
            serial::printc(*str++);
    }

    void printc(char c)
    {
        term::printc(c);
        serial::printc(c);
    }
} // namespace log

void laihost_log(int level, const char *msg)
{
    switch (level)
    {
        case LAI_DEBUG_LOG:
            log::infoln("LAI: {:c}{}", char(toupper(*msg)), msg + 1);
            break;
        case LAI_WARN_LOG:
            log::warnln("LAI: {:c}{}", char(toupper(*msg)), msg + 1);
            break;
    }
}

extern "C"
{
    void FRG_INTF(log)(const char *msg)
    {
        log::infoln("FRG: {:c}{}", char(toupper(*msg)), msg + 1);
    }
} // extern "C"

extern "C" void putchar_(char c) { log::printc(c); }