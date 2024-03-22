// Copyright (C) 2022-2024  ilobilo

#include <drivers/serial.hpp>
#include <drivers/term.hpp>
#include <lib/log.hpp>

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

extern "C"
{
    void putchar_(char c) { log::printc(c); }
} // extern "C"