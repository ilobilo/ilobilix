// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

namespace cpu
{
    // TODO: Registers
    struct registers_t
    {
        uint64_t x[31];
    };

    void set_el1_base(uintptr_t base);
    uintptr_t get_el1_base();

    void set_el0_base(uintptr_t base);
    uintptr_t get_el0_base();
} // namespace cpu