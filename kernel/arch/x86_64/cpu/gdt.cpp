// Copyright (C) 2022-2023  ilobilo

#include <arch/x86_64/cpu/gdt.hpp>
#include <init/kernel.hpp>
#include <lib/lock.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/pmm.hpp>

namespace gdt
{
    TSS *tss = nullptr;
    static lock_t lock;

    void init(size_t num)
    {
        lockit(lock);
        if (tss == nullptr)
        {
            log::infoln("GDT: Initialising...");
            tss = new TSS[smp_request.response->cpu_count];
        }

        uintptr_t base = reinterpret_cast<uintptr_t>(&tss[num]);
        uint16_t limit = sizeof(tss[num]);

        GDT *gdt = new GDT
        {
            { 0x0000, 0, 0, 0x00, 0x00, 0 }, // Null
            { 0x0000, 0, 0, 0x9A, 0x20, 0 }, // Kernel code
            { 0x0000, 0, 0, 0x92, 0x00, 0 }, // Kernel data
            { 0x0000, 0, 0, 0xF2, 0x00, 0 }, // User data
            { 0x0000, 0, 0, 0xFA, 0x20, 0 }, // User code
            {
                limit,
                static_cast<uint16_t>(base),
                static_cast<uint8_t>(base >> 16),
                0x89,
                0x00,
                static_cast<uint8_t>(base >> 24),
                static_cast<uint32_t>(base >> 32),
                0x00
            } // Tss
        };

        tss[num].RSP[0] = tohh(pmm::alloc<uint64_t>(kernel_stack_size / pmm::page_size)) + kernel_stack_size;

        GDTR gdtr
        {
            sizeof(GDT) - 1,
            reinterpret_cast<uintptr_t>(gdt),
        };

        asm volatile (
            "lgdt %[gdtr]\n\t"
            "mov %[dsel], %%ds \n\t"
            "mov %[dsel], %%fs \n\t"
            "mov %[dsel], %%gs \n\t"
            "mov %[dsel], %%es \n\t"
            "mov %[dsel], %%ss \n\t"
            "push %[csel] \n\t"
            "lea 1f(%%rip), %%rax \n\t"
            "push %%rax \n\t"
            ".byte 0x48, 0xCB \n"
            "1:"
            :: [gdtr]"m"(gdtr), [dsel]"m"(GDT_DATA), [csel]"i"(GDT_CODE)
            : "rax", "memory"
        );
        asm volatile ("ltr %0" :: "r"(static_cast<uint16_t>(GDT_TSS)));
    }
} // namespace gdt