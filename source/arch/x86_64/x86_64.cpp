// Copyright (C) 2022  ilobilo

#if defined(__x86_64__) || defined(_M_X64)

#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/idt/idt.hpp>
#include <arch/x86_64/pic/pic.hpp>
#include <arch/x86_64/vmm/vmm.hpp>
#include <lib/log.hpp>

namespace arch::x86_64
{
    void init()
    {
        log::info("Initialising GDT... ");
        gdt::init();
        log::println("Done!");

        log::info("Initialising IDT... ");
        idt::init();
        log::println("Done!");

        log::info("Initialising PIC... ");
        pic::init();
        log::println("Done!");
    }
} // namespace arch::x86_64

#endif