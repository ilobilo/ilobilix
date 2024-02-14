// Copyright (C) 2022-2024  ilobilo

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

    static inline constexpr uint64_t tlbi(uint16_t asid, uintptr_t address)
    {
        return (uint64_t(asid) << 48) | (address >> 12);
    }

    void invlpg(uintptr_t address)
    {
        asm volatile (
            "dsb st; \n\t"
            "tlbi vale1, %0;\n\t"
            "dsb sy; isb"
            :: "r"(tlbi(0, address))
            : "memory");
    }

    void invlpg(uint16_t asid, uintptr_t address)
    {
        asm volatile (
            "dsb st; \n\t"
            "tlbi vae1, %0;\n\t"
            "dsb sy; isb"
            :: "r"(tlbi(asid, address))
            : "memory");
    }
} // namespace cpu