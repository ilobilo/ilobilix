// Copyright (C) 2022  ilobilo

#include <drivers/term/term.hpp>
#include <lib/log.hpp>
#include <lai/host.h>
#include <cstddef>
#include <utility>

[[noreturn]] void halt()
{
    while (true)
    {
        #if defined(__x86_64__)
        asm volatile ("cli; hlt");
        #endif
    }
}

[[noreturn]] void panic(const char *message, const char *file, const char *func, int line)
{
    log::println();
    log::error("PANIC: %s", message);
    log::error("File: %s", file);
    log::error("Function: %s", func);
    log::error("Line: %d", line);
    log::error("System halted!\n");

    printf("\n[\033[31mPANIC\033[0m] %s", message);
    printf("\n[\033[31mPANIC\033[0m] File: %s", file);
    printf("\n[\033[31mPANIC\033[0m] Function: %s", func);
    printf("\n[\033[31mPANIC\033[0m] Line: %d", line);
    printf("\n[\033[31mPANIC\033[0m] System halted!\n");

    halt();
}

[[noreturn]] extern "C" void abort()
{
    log::println();
    log::error("PANIC: abort()");
    log::error("System halted!\n");

    printf("\n[\033[31mPANIC\033[0m] abort()");
    printf("\n[\033[31mPANIC\033[0m] System halted!\n");

    halt();
}

[[gnu::noreturn]] void laihost_panic(const char *msg)
{
    log::error("%s", msg);
    while (true)
    {
        #if defined(__x86_64__)
        asm volatile ("cli; hlt");
        #endif
    }
}