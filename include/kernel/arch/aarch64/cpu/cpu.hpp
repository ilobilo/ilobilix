// Copyright (C) 2022-2023  ilobilo

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

    void invlpg(uintptr_t address);
    void invlpg(uint16_t asid, uintptr_t address);

    #define read_ttbr_el1(num)                                   \
    ({                                                           \
        uint64_t value;                                          \
        asm volatile ("mrs %0, ttbr" #num "_el1" : "=r"(value)); \
        value;                                                   \
    })

    #define write_ttbr_el1(num, value)                           \
    {                                                            \
        asm volatile ("msr ttbr" #num "_el1, %0" :: "r"(value)); \
    }
} // namespace cpu