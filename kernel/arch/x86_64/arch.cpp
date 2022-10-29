// Copyright (C) 2022  ilobilo

#include <arch/x86_64/drivers/timers/hpet.hpp>
#include <arch/x86_64/drivers/timers/pit.hpp>
#include <arch/x86_64/drivers/timers/rtc.hpp>

#include <arch/x86_64/cpu/ioapic.hpp>
#include <arch/x86_64/cpu/gdt.hpp>
#include <arch/x86_64/cpu/idt.hpp>
#include <arch/x86_64/cpu/pic.hpp>
#include <arch/x86_64/lib/io.hpp>

#include <drivers/pci/pci.hpp>
#include <drivers/smp.hpp>
#include <arch/arch.hpp>

namespace arch
{
    [[noreturn]] void halt(bool ints)
    {
        if (ints == true)
            while (true)
                asm volatile ("hlt");
        else
            while (true)
                asm volatile ("cli; hlt");
    }

    void wfi()
    {
        asm volatile ("hlt");
    }

    void pause()
    {
        asm volatile ("pause");
    }

    void int_toggle(bool on)
    {
        if (on == true)
            asm volatile ("sti");
        else
            asm volatile ("cli");
    }

    bool int_status()
    {
        uint64_t rflags = 0;
        asm volatile (
            "pushfq \n\t"
            "pop %[rflags]"
            : [rflags]"=r"(rflags)
        );
        return rflags & (1 << 9);
    }

    std::optional<uint64_t> time_ns()
    {
        if (timers::hpet::initialised == true)
            return timers::hpet::time_ns();

        return std::nullopt;
    }

    uint64_t epoch()
    {
        using namespace timers::rtc;
        return ::epoch(second(), minute(), hour(), day(), month(), year(), century());
    }

    // TODO
    void shutdown(bool now);
    void reboot(bool now);

    void init()
    {
        smp::bsp_init();

        pic::init();
        ioapic::init();

        timers::pit::init();
        timers::hpet::init();

        smp::init();
    }
} // namespace arch