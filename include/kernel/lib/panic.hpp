// Copyright (C) 2022  ilobilo

#pragma once

#include <format>

namespace cpu { struct registers_t; }

[[noreturn]] void panic(const char *file, int line, const char *func, const char *message);
[[noreturn]] void panic(const char *message);

[[noreturn]] void vpanic(cpu::registers_t *regs, uintptr_t bp, uintptr_t fip, std::string_view format, fmt::format_args args);

[[noreturn]] void panic(cpu::registers_t *regs, std::string_view format, auto &&...args)
{
    uintptr_t bp = reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
    vpanic(regs, bp, 0, format, fmt::make_format_args(args...));
}
[[noreturn]] void panic(cpu::registers_t *regs, uintptr_t bp, uintptr_t fip, std::string_view format, auto &&...args)
{
    vpanic(regs, bp, fip, format, fmt::make_format_args(args...));
}

extern "C" [[noreturn]] void abort() noexcept;

#define PANIC(msg) panic(__FILE__, __LINE__, __PRETTY_FUNCTION__, msg)