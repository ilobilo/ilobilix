// Copyright (C) 2022  ilobilo

#include <arch/x86_64/cpu/cpu.hpp>
#include <lib/mmio.hpp>
#include <lib/log.hpp>

namespace cpu
{
    bool id(uint32_t leaf, uint32_t subleaf, uint32_t &eax, uint32_t &ebx, uint32_t &ecx, uint32_t &edx)
    {
        uint32_t cpuid_max = 0;
        asm volatile ("cpuid" : "=a"(cpuid_max) : "a"(leaf & 0x80000000) : "ebx", "ecx", "edx");
        if (leaf > cpuid_max) return false;
        asm volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(leaf), "c"(subleaf));
        return true;
    }

    uint64_t rdmsr(uint32_t msr)
    {
        uint32_t edx, eax;
        asm volatile ("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr) : "memory");
        return (static_cast<uint64_t>(edx) << 32) | eax;
    }

    void wrmsr(uint32_t msr, uint64_t value)
    {
        uint32_t edx = value >> 32;
        uint32_t eax = static_cast<uint32_t>(value);
        asm volatile ("wrmsr" :: "a"(eax), "d"(edx), "c"(msr) : "memory");
    }

    void set_kernel_gs(uint64_t addr)
    {
        wrmsr(0xC0000102, addr);
    }

    uint64_t get_kernel_gs()
    {
        return rdmsr(0xC0000102);
    }

    void set_gs(uint64_t addr)
    {
        wrmsr(0xC0000101, addr);
    }

    uint64_t get_gs()
    {
        return rdmsr(0xC0000101);
    }

    void set_fs(uint64_t addr)
    {
        wrmsr(0xC0000100, addr);
    }

    uint64_t get_fs()
    {
        return rdmsr(0xC0000100);
    }

    void wrxcr(uint32_t i, uint64_t value)
    {
        uint32_t edx = value >> 32;
        uint32_t eax = static_cast<uint32_t>(value);
        asm volatile ("xsetbv" :: "a"(eax), "d"(edx), "c"(i) : "memory");
    }

    static uint64_t rfbm = ~0ULL;
    static uint32_t rfbm_low = rfbm & 0xFFFFFFFF;
    static uint32_t rfbm_high = (rfbm >> 32) & 0xFFFFFFFF;

    void xsaveopt(uint8_t *region)
    {
        asm volatile ("xsaveopt64 (%0)" :: "r"(region), "a"(rfbm_low), "d"(rfbm_high) : "memory");
    }

    void xsave(uint8_t *region)
    {
        asm volatile ("xsaveq (%0)" :: "r"(region), "a"(rfbm_low), "d"(rfbm_high) : "memory");
    }

    void xrstor(uint8_t *region)
    {
        asm volatile ("xrstorq (%0)" :: "r"(region), "a"(rfbm_low), "d"(rfbm_high) : "memory");
    }

    void fxsave(uint8_t *region)
    {
        asm volatile ("fxsaveq (%0)" :: "r"(region) : "memory");
    }

    void fxrstor(uint8_t *region)
    {
        asm volatile ("fxrstorq (%0)" :: "r"(region) : "memory");
    }

    void invlpg(uint64_t addr)
    {
        asm volatile ("invlpg (%0)" :: "r"(addr));
    }

    void stac()
    {
        if (read_cr(4) & (1 << 21))
            asm volatile ("stac" ::: "cc");
    }

    void clac()
    {
        if (read_cr(4) & (1 << 21))
            asm volatile ("clac" ::: "cc");
    }

    void enableSSE()
    {
        write_cr(0, (read_cr(0) & ~(1 << 2)) | (1 << 1));
        write_cr(4, read_cr(4) | (3 << 9));
    }

    void enablePAT()
    {
        wrmsr(0x277, custom_pat);
    }

    void enableSMEP()
    {
        uint32_t a = 0, b = 0, c = 0, d = 0;
        if (cpu::id(7, 0, a, b, c, d))
        {
            if (b & CPUID_SMEP)
                write_cr(4, read_cr(4) | (1 << 20));
        }
    }

    void enableSMAP()
    {
        uint32_t a = 0, b = 0, c = 0, d = 0;
        if (cpu::id(7, 0, a, b, c, d))
        {
            if (b & CPUID_SMAP)
            {
                write_cr(4, read_cr(4) | (1 << 21));
                clac();
            }
        }
    }

    void enableUMIP()
    {
        uint32_t a = 0, b = 0, c = 0, d = 0;
        if (cpu::id(7, 0, a, b, c, d))
        {
            if (c & CPUID_UMIP)
                write_cr(4, read_cr(4) | (1 << 11));
        }
    }
} // namespace cpu