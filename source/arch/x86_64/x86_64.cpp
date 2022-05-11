// Copyright (C) 2022  ilobilo

#if defined(__x86_64__) || defined(_M_X64)

#include <arch/x86_64/timers/lapic/lapic.hpp>
#include <arch/x86_64/timers/hpet/hpet.hpp>
#include <arch/x86_64/timers/pit/pit.hpp>
#include <arch/x86_64/timers/pit/pit.hpp>
#include <arch/x86_64/apic/apic.hpp>
#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/idt/idt.hpp>
#include <arch/x86_64/pic/pic.hpp>
#include <arch/x86_64/vmm/vmm.hpp>
#include <lib/log.hpp>

namespace arch::x86_64
{
    void init()
    {
        log::info("Initialising GDT...");
        gdt::init();

        log::info("Initialising IDT...");
        idt::init();

        log::info("Initialising PIC...");
        pic::init();

        log::info("Initialising APIC...");
        apic::init();

        log::info("Initialising PIT...");
        timers::pit::init();

        log::info("Initialising HPET...");
        timers::hpet::init();

        log::info("Initialising LAPIC Timer...");
        timers::lapic::init();
    }
} // namespace arch::x86_64

#endif