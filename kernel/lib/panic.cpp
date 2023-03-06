// Copyright (C) 2022-2023  ilobilo

#include <drivers/proc.hpp>
#include <frg/macros.hpp>
#include <arch/arch.hpp>
#include <lib/panic.hpp>
#include <lib/trace.hpp>
#include <cpu/cpu.hpp>
#include <lib/log.hpp>
#include <lai/host.h>
#include <cstddef>
#include <utility>

[[noreturn]] void vpanic(const char *file, int line, const char *func, std::string_view format, fmt::format_args args)
{
    log::println();
    log::errorln("{}", fmt::vformat(format, args));
    log::errorln("File: {}", file);
    log::errorln("Line: {}", line);
    log::errorln("Function: {}", func);

    // trace::print(0, 0, log::error_prefix);

    log::errorln("System halted!");
    arch::halt(false);
}

[[noreturn]] void panic(const char *message)
{
    log::println();
    log::errorln("{}", message);

    log::errorln("System halted!");
    arch::halt(false);
}

[[noreturn]] void vpanic(cpu::registers_t *regs, uintptr_t bp, uintptr_t fip, std::string_view format, fmt::format_args args, bool trace)
{
    arch::int_toggle(false);
    arch::halt_others();

    if (log::lock.is_locked())
        log::lock.unlock();

    log::println();
    log::errorln("{}", fmt::vformat(format, args));
    if (proc::initialised == true)
    {
        auto [pid, tid] = proc::pid();
        log::errorln("Process ID: {}, Thread ID: {}", pid, tid);
    }

    log::errorln();
    arch::dump_regs(regs, log::error_prefix);

    if (trace == true)
    {
        log::errorln();
        trace::print(bp, fip, log::error_prefix);
    }

    log::errorln();
    log::errorln("System halted!");

    arch::halt(false);
}

extern "C"
{
    [[gnu::noreturn]] void FRG_INTF(panic)(const char *cstring)
    {
        panic(cstring);
    }

    [[noreturn]] void abort() noexcept
    {
        panic("abort()");
    }

    void assert_fail(const char *message, const char *file, int line, const char *func)
    {
        panic(file, line, func, message);
    }
} // extern "C"

[[gnu::noreturn]] void laihost_panic(const char *msg) { panic(msg); }