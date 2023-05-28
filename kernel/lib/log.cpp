// Copyright (C) 2022-2023  ilobilo

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
            if (to_term == true)
                term::printc(*str);
            serial::printc(*str++);
        }
    }

    void prints(const char *str)
    {
        if (to_term == true)
            term::print(str);
        while (*str)
            serial::printc(*str++);
    }

    void printc(char c)
    {
        if (to_term == true)
            term::printc(c);
        serial::printc(c);
    }
} // namespace log

void laihost_log(int level, const char *msg)
{
    switch (level)
    {
        case LAI_DEBUG_LOG:
            log::infoln("LAI: {}", msg);
            break;
        case LAI_WARN_LOG:
            log::warnln("LAI: {}", msg);
            break;
    }
}

extern "C"
{
    void FRG_INTF(log)(const char *msg)
    {
        log::infoln("FRG: {}{}", char(toupper(*msg)), msg + 1);
    }
    void putchar_(char c) { log::printc(c); }
} // extern "C"