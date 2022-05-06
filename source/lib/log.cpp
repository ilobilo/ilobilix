// Copyright (C) 2022  ilobilo

#include <drivers/serial/serial.hpp>
#include <drivers/term/term.hpp>
#include <lib/lock.hpp>
#include <cstdarg>

namespace log
{
    lock_t lock;

    int print(const char *fmt, ...)
    {
        lockit(lock);

        va_list args;
        va_start(args, fmt);
        int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, args);
        va_end(args);

        return ret;
    }

    int info(const char *fmt, ...)
    {
        lockit(lock);

        va_list args;
        va_start(args, fmt);
        int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "[\033[32mINFO\033[0m] ", args);
        ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, args);
        ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "\n", args);
        va_end(args);

        return ret;
    }

    int warn(const char *fmt, ...)
    {
        lockit(lock);

        va_list args;
        va_start(args, fmt);
        int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "[\033[33mWARN\033[0m] ", args);
        ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, args);
        ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "\n", args);
        va_end(args);

        return ret;
    }

    int error(const char *fmt, ...)
    {
        lockit(lock);

        va_list args;
        va_start(args, fmt);
        int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "[\033[31mERROR\033[0m] ", args);
        ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, args);
        ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "\n", args);
        va_end(args);

        return ret;
    }
} // namespace log