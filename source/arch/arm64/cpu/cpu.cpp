// Copyright (C) 2022  ilobilo

#if defined(__aarch64__)

#include <arch/arm64/cpu/cpu.hpp>

namespace arch::arm64::cpu
{
    void set_base(uint64_t addr)
    {
        asm volatile ("msr TPIDR_EL1, %0" :: "r"(addr) : "memory");
    }

    uint64_t get_base()
    {
        uint64_t addr = 0;
        asm volatile ("mrs %0, TPIDR_EL1" : "=r"(addr) :: "memory");
        return addr;
    }
} // namespace arch::arm64::cpu

#endif