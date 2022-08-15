// Copyright (C) 2022  ilobilo

#include <frg/macros.hpp>
#include <arch/arch.hpp>
#include <lib/log.hpp>
#include <lai/host.h>
#include <cstddef>
#include <utility>

[[noreturn]] void panic(const char *file, int line, const char *func, const char *message)
{
    log::println();
    log::error("%s", message);
    log::error("File: %s", file);
    log::error("Line: %d", line);
    log::error("Function: %s", func);
    log::error("System halted!");

    arch::halt(false);
}

[[noreturn]] void panic(const char *message)
{
    log::println();
    log::error("%s", message);
    log::error("System halted!");

    arch::halt(false);
}

extern "C"
{
    [[gnu::noreturn]] void FRG_INTF(panic)(const char *cstring)
    {
        panic(cstring);
    }

    [[noreturn]] void abort()
    {
        panic("abort()");
    }

    void assert_fail(const char *message, const char *file, int line, const char *func)
    {
        panic(file, line, func, message);
    }
} // extern "C"

[[gnu::noreturn]] void laihost_panic(const char *msg) { panic(msg); }