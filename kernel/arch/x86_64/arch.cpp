// Copyright (C) 2022-2023  ilobilo

#include <arch/x86_64/drivers/timers/hpet.hpp>
#include <arch/x86_64/drivers/timers/pit.hpp>
#include <arch/x86_64/drivers/timers/rtc.hpp>

#include <arch/x86_64/syscall/syscall.hpp>

#include <arch/x86_64/cpu/ioapic.hpp>
#include <arch/x86_64/cpu/gdt.hpp>
#include <arch/x86_64/cpu/idt.hpp>
#include <arch/x86_64/cpu/pic.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/lib/io.hpp>

#include <drivers/pci/pci.hpp>
#include <drivers/ps2/ps2.hpp>
#include <drivers/smp.hpp>
#include <arch/arch.hpp>

#include <lib/interrupts.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>

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

    void halt_others()
    {
        if (smp::initialised == true)
            this_cpu()->lapic.ipi(idt::panic_int | (0b10 << 18), 0);
    }

    void dump_regs(cpu::registers_t *regs, const char *prefix)
    {
        log::println("{}CPU context:", prefix);
        log::println("{}  R15: 0x{:016X}, R14: 0x{:016X}", prefix, regs->r15, regs->r14);
        log::println("{}  R13: 0x{:016X}, R12: 0x{:016X}", prefix, regs->r13, regs->r12);
        log::println("{}  R11: 0x{:016X}, R10: 0x{:016X}", prefix, regs->r11, regs->r10);
        log::println("{}  R9:  0x{:016X}, R8:  0x{:016X}", prefix, regs->r9, regs->r8);
        log::println("{}  RBP: 0x{:016X}, RDI: 0x{:016X}", prefix, regs->rbp, regs->rdi);
        log::println("{}  RSI: 0x{:016X}, RDX: 0x{:016X}", prefix, regs->rsi, regs->rdx);
        log::println("{}  RCX: 0x{:016X}, RBX: 0x{:016X}", prefix, regs->rcx, regs->rbx);
        log::println("{0}  RAX: 0x{1:016X}, ERR: 0x{2:X} : 0b{2:b}", prefix, regs->rax, regs->error_code);
        log::println("{}  RSP: 0x{:016X}, RIP: 0x{:016X}", prefix, regs->rsp, regs->rip);
        log::println("{}  RFLAGS: 0x{:X}, CS: 0x{:X}, SS: 0x{:X}", prefix, regs->rflags, regs->cs, regs->ss);
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

    [[noreturn]] void shutdown()
    {
        acpi::poweroff();
        halt(false);
    }

    [[noreturn]] void reboot()
    {
        // try acpi reboot twice
        acpi::reboot();
        pause();
        acpi::reboot();
        pause();

        // ps2 reset
        uint8_t good = 0b10;
        while (good & 0b10)
            good = io::in<uint8_t>(0x64);
        io::out<uint8_t>(0x64, 0xFE);
        pause();

        // triple fault
        idt::invalid.load();
        asm volatile ("int3");

        halt(false);
    }

    void early_init()
    {
        smp::bsp_init();

        pic::init();
        ioapic::init();

        timers::pit::init();
        timers::hpet::init();

        syscall::init();

        smp::init();
    }

    void init()
    {
        ps2::init();
    }
} // namespace arch

namespace interrupts
{
    std::pair<handler&, size_t> allocate_handler()
    {
        return idt::allocate_handler();
    }

    handler &get_handler(size_t vector)
    {
        return idt::handlers[vector];
    }
} // namespace interrupts