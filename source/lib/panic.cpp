// Copyright (C) 2022  ilobilo

#include <drivers/term/term.hpp>
#include <lib/log.hpp>
#include <cstddef>
#include <utility>

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

    while (true) asm volatile ("cli; hlt");
}

[[noreturn]] extern "C" void abort()
{
    log::error("PANIC: abort()\n");
    log::error("System halted!\n\n");

    printf("\n[\033[31mPANIC\033[0m] abort()");
    printf("\n[\033[31mPANIC\033[0m] System halted!\n");

    while (true) asm volatile ("cli; hlt");
}