// Copyright (C) 2022  ilobilo

#include <arch/aarch64/cpu/cpu.hpp>

namespace cpu
{
    void set_el1_base(uintptr_t base)
    {
        asm volatile ("msr tpidr_el1, %0" :: "r"(base) : "memory");
    }

    uintptr_t get_el1_base()
    {
        uintptr_t base = 0;
        asm volatile ("mrs %0, tpidr_el1" : "=r"(base) :: "memory");
        return base;
    }

    void set_el0_base(uintptr_t base)
    {
        asm volatile ("msr tpidr_el0, %0" :: "r"(base) : "memory");
    }

    uintptr_t get_el0_base()
    {
        uintptr_t base = 0;
        asm volatile ("mrs %0, tpidr_el0" : "=r"(base) :: "memory");
        return base;
    }
} // namespace cpu