// Copyright (C) 2022  ilobilo

#pragma once

#include <optional>
#include <cstdint>

namespace arch
{
    [[noreturn]] void halt(bool ints = true);
    void wfi();
    void pause();

    void int_toggle(bool on);
    bool int_status();

    std::optional<uint64_t> time_ns();
    uint64_t epoch();

    void shutdown(bool now);
    void reboot(bool now);

    void init();
} // namespace arch