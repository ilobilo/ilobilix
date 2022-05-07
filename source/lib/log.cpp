// Copyright (C) 2022  ilobilo

#include <drivers/uart/uart.hpp>
#include <drivers/term/term.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <cstdarg>

namespace log
{
    static lock_t lock;

    int print(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vfctprintf(uart::printc, nullptr, fmt, arg);

        va_end(arg);
        return ret;
    }

    int println(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vfctprintf(uart::printc, nullptr, fmt, arg);
        ret += vfctprintf(uart::printc, nullptr, "\n", arg);

        va_end(arg);
        return ret;
    }

    int info(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vfctprintf(uart::printc, nullptr, info_prefix, arg);
        ret += vfctprintf(uart::printc, nullptr, fmt, arg);

        va_end(arg);
        return ret;
    }

    int warn(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vfctprintf(uart::printc, nullptr, warn_prefix, arg);
        ret += vfctprintf(uart::printc, nullptr, fmt, arg);

        va_end(arg);
        return ret;
    }

    int error(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vfctprintf(uart::printc, nullptr, error_prefix, arg);
        ret += vfctprintf(uart::printc, nullptr, fmt, arg);

        va_end(arg);
        return ret;
    }
} // namespace log