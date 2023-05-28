// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <optional>
#include <cstdint>

namespace cpu { struct registers_t; }
namespace arch
{
    [[noreturn]] void halt(bool ints = true);
    void halt_others();

    void dump_regs(cpu::registers_t *regs, const char *prefix);

    void wfi();
    void pause();

    void int_toggle(bool on);
    bool int_status();

    std::optional<uint64_t> time_ns();
    uint64_t epoch();

    [[noreturn]] void shutdown();
    [[noreturn]] void reboot();

    void init();
    void late_init();
} // namespace arch