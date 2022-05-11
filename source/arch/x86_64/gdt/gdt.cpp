// Copyright (C) 2022  ilobilo

#if defined(__x86_64__) || defined(_M_X64)

#include <arch/x86_64/gdt/gdt.hpp>
#include <lib/lock.hpp>
#include <main.hpp>

namespace arch::x86_64::gdt
{
    TSS *tss = nullptr;
    GDTDescriptor gdtr;
    static lock_t lock;
    GDT gdt
    {
        { 0x0000, 0, 0, 0x00, 0x00, 0 }, // Null
        { 0x0000, 0, 0, 0x9A, 0x20, 0 }, // Kernel code
        { 0x0000, 0, 0, 0x92, 0x00, 0 }, // Kernel data
        { 0x0000, 0, 0, 0xFA, 0x20, 0 }, // User code
        { 0x0000, 0, 0, 0xF2, 0x00, 0 }, // User data
        { 0x0000, 0, 0, 0x89, 0x00, 0, 0, 0 } // Tss
    };

    void initcpu(size_t num)
    {
        lockit(lock);

        uintptr_t base = reinterpret_cast<uintptr_t>(&tss[num]);
        uintptr_t limit = base + sizeof(tss[num]);

        gdt.Tss.Length = limit;

        gdt.Tss.Base0 = base;
        gdt.Tss.Base1 = base >> 16;
        gdt.Tss.Flags1 = 0x89;
        gdt.Tss.Flags2 = 0x00;
        gdt.Tss.Base2 = base >> 24;
        gdt.Tss.Base3 = base >> 32;
        gdt.Tss.Reserved = 0x00;

        asm volatile ("lgdt %0" :: "m"(gdtr) : "memory");
        asm volatile (
            "mov %[dsel], %%ds \n\t"
            "mov %[dsel], %%fs \n\t"
            "mov %[dsel], %%gs \n\t"
            "mov %[dsel], %%es \n\t"
            "mov %[dsel], %%ss \n\t"
            :: [dsel]"rm"(GDT_DATA)
        );
        asm volatile (
            "push %[csel] \n\t"
            "lea 1f(%%rip), %%rax \n\t"
            "push %%rax \n\t"
            ".byte 0x48, 0xCB \n\t"
            "1:"
            :: [csel]"i"(GDT_CODE) : "rax"
        );
        asm volatile ("ltr %0" :: "r"(static_cast<uint16_t>(GDT_TSS)));
    }

    void init()
    {
        tss = new TSS[smp_request.response->cpu_count];
        gdtr.Offset = reinterpret_cast<uint64_t>(&gdt);
        gdtr.Size = sizeof(GDT) - 1;

        initcpu(0);
    }
} // namespace arch::x86_64::gdt

#endif