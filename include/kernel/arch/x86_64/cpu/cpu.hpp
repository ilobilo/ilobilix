// Copyright (C) 2022  ilobilo

#pragma once

#include <concepts>
#include <utility>
#include <cstdint>

namespace cpu
{
    static constexpr uint64_t CPUID_SMEP = (1 << 7);
    static constexpr uint64_t CPUID_SMAP = (1 << 20);
    static constexpr uint64_t CPUID_UMIP = (1 << 2);

    static constexpr uint64_t Uncachable = 0x00;
    static constexpr uint64_t WriteCombining = 0x01;
    static constexpr uint64_t WriteThrough = 0x04;
    static constexpr uint64_t WriteProtected = 0x05;
    static constexpr uint64_t WriteBack = 0x06;
    static constexpr uint64_t Uncached = 0x07;

    static constexpr uint64_t reset_state_pat = 0x00'07'04'06'00'07'04'06; // UC UC- WT WB UC UC- WT WB
    static constexpr uint64_t custom_pat = Uncachable | (WriteCombining << 8) | (WriteThrough << 32) | (WriteProtected << 40) | (WriteBack << 48) | (Uncached << 56);

    struct [[gnu::packed]] registers_t
    {
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
        uint64_t int_no, error_code, rip, cs, rflags, rsp, ss;
    };

    bool id(uint32_t leaf, uint32_t subleaf, uint32_t &eax, uint32_t &ebx, uint32_t &ecx, uint32_t &edx);

    uint64_t rdmsr(uint32_t msr);
    void wrmsr(uint32_t msr, uint64_t value);

    void set_kernel_gs(uint64_t addr);
    uint64_t get_kernel_gs();

    void set_gs(uint64_t addr);
    uint64_t get_gs();

    void set_fs(uint64_t addr);
    uint64_t get_fs();

    void wrxcr(uint32_t i, uint64_t value);

    void xsaveopt(uint8_t *region);
    void xsave(uint8_t *region);
    void xrstor(uint8_t *region);
    void fxsave(uint8_t *region);
    void fxrstor(uint8_t *region);

    void invlpg(uint64_t addr);

    void stac();
    void clac();

    void enableSSE();
    void enablePAT();

    void enableSMEP();
    void enableSMAP();
    void enableUMIP();

    struct as_user_t
    {
        as_user_t() { stac(); }
        ~as_user_t() { clac(); }
    };

    template<typename Func, typename ...Args>
        requires (!std::same_as<std::invoke_result_t<Func, Args...>, void>)
    static decltype(auto) as_user(Func &&func, Args &&...args)
    {
        as_user_t guard;
        return func(std::forward<Args>(args)...);
    }

    template<typename Func, typename ...Args>
    static void as_user(Func &&func, Args &&...args)
    {
        as_user_t guard;
        func(std::forward<Args>(args)...);
    }

    #define rdreg(reg)                                               \
    ({                                                               \
        uintptr_t val;                                               \
        asm volatile ("mov %%" #reg ", %0" : "=r"(val) :: "memory"); \
        val;                                                         \
    })

    #define wrreg(reg, val) asm volatile ("mov %0, %%" #reg :: "r"(val) : "memory");
} // namespace cpu