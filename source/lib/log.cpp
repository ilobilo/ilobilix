// Copyright (C) 2022  ilobilo

#include <drivers/serial/serial.hpp>
#include <drivers/term/term.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <cstdarg>

namespace log
{
    lock_t lock;

    int print(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, arg);

        va_end(arg);
        return ret;
    }

    int println(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, arg);
        ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "\n", arg);

        va_end(arg);
        return ret;
    }

    int info(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), info_prefix, arg);
        ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, arg);

        va_end(arg);
        return ret;
    }

    int warn(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), warn_prefix, arg);
        ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, arg);

        va_end(arg);
        return ret;
    }

    int error(const char *fmt, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, fmt);

        int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), error_prefix, arg);
        ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, arg);

        va_end(arg);
        return ret;
    }
} // namespace log