// Copyright (C) 2022-2024  ilobilo

#include <arch/x86_64/cpu/gdt.hpp>
#include <init/kernel.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/pmm.hpp>

namespace gdt
{
    tss::ptr *tsses = nullptr;
    static std::mutex lock;

    void init(size_t num)
    {
        std::unique_lock guard(lock);
        if (tsses == nullptr)
        {
            log::infoln("GDT: Initialising...");
            tsses = new tss::ptr[smp_request.response->cpu_count];
        }
        tsses[num].rsp[0] = tohh(pmm::alloc<uint64_t>(kernel_stack_size / pmm::page_size)) + kernel_stack_size;

        struct [[gnu::packed]] entries
        {
            entry null;
            entry kcode;
            entry kdata;
            entry ucode;
            entry udata;
            tss::entry tss;
        };

        uintptr_t base = reinterpret_cast<uintptr_t>(&tsses[num]);
        uint16_t limit = sizeof(tsses[num]);

        ptr gdtr
        {
            sizeof(entries) - 1,
            reinterpret_cast<uintptr_t>(
                new entries {
                    { 0x0000, 0, 0, 0x00, 0x00, 0 }, // null
                    { 0x0000, 0, 0, 0x9A, 0x20, 0 }, // kernel code
                    { 0x0000, 0, 0, 0x92, 0x00, 0 }, // kernel data
                    { 0x0000, 0, 0, 0xF2, 0x00, 0 }, // user data
                    { 0x0000, 0, 0, 0xFA, 0x20, 0 }, // user code
                    { // tss
                        limit,
                        static_cast<uint16_t>(base),
                        static_cast<uint8_t>(base >> 16),
                        0x89, 0x00,
                        static_cast<uint8_t>(base >> 24),
                        static_cast<uint32_t>(base >> 32),
                        0x00
                    }
                }
            ),
        };

        asm volatile (
            "lgdt %[gdtr] \n\t"
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