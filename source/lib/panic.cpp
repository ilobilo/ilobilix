// Copyright (C) 2022  ilobilo

#include <drivers/term/term.hpp>
#include <lib/log.hpp>
#include <cstddef>
#include <utility>

[[noreturn]] void halt()
{
    while (true)
    {
        #if defined(__x86_64__) || defined(_M_X64)
        asm volatile ("cli; hlt");
        #endif
    }
}

[[noreturn]] void panic(const char *message, const char *file, const char *func, int line)
{
    log::error("PANIC: %s\n", message);
    log::error("File: %s\n", file);
    log::error("Function: %s\n", func);
    log::error("Line: %d\n", line);
    log::error("System halted!\n\n");

    printf("\n[\033[31mPANIC\033[0m] %s", message);
    printf("\n[\033[31mPANIC\033[0m] File: %s", file);
    printf("\n[\033[31mPANIC\033[0m] Function: %s", func);
    printf("\n[\033[31mPANIC\033[0m] Line: %d", line);
    printf("\n[\033[31mPANIC\033[0m] System halted!\n");

    halt();
}

[[noreturn]] extern "C" void abort()
{
    log::error("PANIC: abort()\n");
    log::error("System halted!\n\n");

    printf("\n[\033[31mPANIC\033[0m] abort()");
    printf("\n[\033[31mPANIC\033[0m] System halted!\n");

    halt();
}