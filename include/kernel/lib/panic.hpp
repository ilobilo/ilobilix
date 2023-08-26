// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <fmt/core.h>

namespace cpu { struct registers_t; }

// TODO: this is a mess

[[noreturn]] void vpanic(const char *file, int line, const char *func, std::string_view format, fmt::format_args args);
[[noreturn]] inline void panic(const char *file, int line, const char *func, std::string_view format, auto &&...args)
{
    vpanic(file, line, func, format, fmt::make_format_args(args...));
}
[[noreturn]] void panic(const char *message);

[[noreturn]] void vpanic(cpu::registers_t *regs, uintptr_t bp, uintptr_t fip, std::string_view format, fmt::format_args args, bool trace = true);
[[noreturn]] inline void panic(cpu::registers_t *regs, uintptr_t bp, uintptr_t fip, std::string_view format, auto &&...args)
{
    vpanic(regs, bp, fip, format, fmt::make_format_args(args...));
}
[[noreturn]] inline void panic(cpu::registers_t *regs, std::string_view format, auto &&...args)
{
    vpanic(regs, 0, 0, format, fmt::make_format_args(args...), false);
}

extern "C" [[noreturn]] void abort() noexcept;

#define PANIC(msg, ...) panic(__FILE__, __LINE__, __PRETTY_FUNCTION__, msg __VA_OPT__(,) __VA_ARGS__)