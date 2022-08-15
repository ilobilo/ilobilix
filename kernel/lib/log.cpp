// Copyright (C) 2022  ilobilo

#include <drivers/serial.hpp>
#include <drivers/term.hpp>
#include <frg/macros.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <lai/host.h>

namespace log
{
    static lock_t lock;
    bool toterm = true;

    void printc(char c, void* = nullptr)
    {
        serial::printc(c);
        if (toterm == true)
            term::printf(term::main_term, "%c", c);
    }

    int vprint(const char *fmt, va_list arg)
    {
        return vfctprintf(printc, nullptr, fmt, arg);
    }

    int print(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vprint(fmt, arg);

        va_end(arg);
        return ret;
    }

    int println(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vprint(fmt, arg);
        printc('\n');

        va_end(arg);
        return ++ret;
    }

    int info(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vprint(info_prefix, arg);
        ret += vprint(fmt, arg);
        printc('\n');

        va_end(arg);
        return ++ret;
    }

    int warn(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vprint(warn_prefix, arg);
        ret += vprint(fmt, arg);
        printc('\n');

        va_end(arg);
        return ++ret;
    }

    int error(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vprint(error_prefix, arg);
        ret += vprint(fmt, arg);
        printc('\n');

        va_end(arg);
        return ++ret;
    }
} // namespace log

void laihost_log(int level, const char *msg)
{
    switch (level)
    {
        case LAI_DEBUG_LOG:
            log::info("%s", msg);
            break;
        case LAI_WARN_LOG:
            log::warn("%s", msg);
            break;
    }
}

extern "C"
{
    void FRG_INTF(log)(const char *cstring)
    {
        log::info("%s", cstring);
    }
} // extern "C"