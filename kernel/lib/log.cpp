// Copyright (C) 2022  ilobilo

#include <drivers/serial.hpp>
#include <drivers/term.hpp>
#include <frg/macros.hpp>
#include <lib/log.hpp>
#include <lai/host.h>

namespace log
{
    static bool toterm = true;

    void prints(const char *str, size_t length)
    {
        while (length--)
        {
            if (toterm == true)
                term::printc(*str);
            serial::printc(*str++);
        }
    }

    void prints(const char *str)
    {
        if (toterm == true)
            term::print(str);

        while (*str)
            serial::printc(*str++);
    }

    void printc(char c)
    {
        if (toterm == true)
            term::printc(c);
        serial::printc(c);
    }

    void toggle_term(bool on)
    {
        lockit(lock);
        toterm = on;
    }

    bool to_term()
    {
        lockit(lock);
        return toterm;
    }
} // namespace log

void laihost_log(int level, const char *msg)
{
    switch (level)
    {
        case LAI_DEBUG_LOG:
            log::infoln("LAI: {}{}", char(toupper(*msg)), msg + 1);
            break;
        case LAI_WARN_LOG:
            log::warnln("LAI: {}{}", char(toupper(*msg)), msg + 1);
            break;
    }
}

extern "C"
{
    void FRG_INTF(log)(const char *msg)
    {
        log::infoln("FRG: {}{}", char(toupper(*msg)), msg + 1);
    }
} // extern "C"