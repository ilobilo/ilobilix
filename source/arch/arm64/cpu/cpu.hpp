// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__aarch64__)

#include <cstdint>

namespace arch::arm64::cpu
{
    void set_base(uint64_t addr);
    uint64_t get_base();
} // namespace arch::arm64::cpu

#endif