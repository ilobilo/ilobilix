// Copyright (C) 2022  ilobilo

#include <arch/x86_64/cpu/ioapic.hpp>
#include <arch/x86_64/cpu/gdt.hpp>
#include <arch/x86_64/cpu/idt.hpp>
#include <arch/x86_64/cpu/pic.hpp>
#include <drivers/acpi.hpp>
#include <drivers/smp.hpp>
#include <init/kernel.hpp>
#include <lib/panic.hpp>
#include <lib/trace.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <cstdio>

namespace idt
{
    int_handler_t handlers[256];
    static lock_t lock;
    uint8_t panic_int;
    IDTEntry idt[256];
    IDTPtr idtr;

    std::pair<int_handler_t&, uint8_t> allocate_handler(uint8_t hint)
    {
        hint = std::max(hint, IRQ(0));

        if (acpi::madthdr->legacy_pic() == true)
        {
            if ((hint >= IRQ(0) && hint <= IRQ(15)) && handlers[hint].get() == false)
                return std::pair<int_handler_t&, uint8_t>(handlers[hint], hint);
        }

        for (size_t i = hint; i < 256; i++)
        {
            if (handlers[i].get() == false && handlers[i].is_reserved() == false)
            {
                handlers[i].reserve();
                return std::pair<int_handler_t&, uint8_t>(handlers[i], i);
            }
        }

        PANIC("Out of interrupt handlers!");
    }

    void mask(uint8_t irq)
    {
        if (ioapic::initialised == true && acpi::madthdr->legacy_pic())
            ioapic::mask_irq(irq);
        else pic::mask(irq);
    }

    void unmask(uint8_t irq)
    {
        if (ioapic::initialised == true && acpi::madthdr->legacy_pic())
            ioapic::unmask_irq(irq);
        else pic::unmask(irq);
    }

    void IDTEntry::set(void *isr, uint8_t typeattr, uint8_t ist)
    {
        uint64_t israddr = reinterpret_cast<uint64_t>(isr);
        this->Offset1 = static_cast<uint16_t>(israddr);
        this->Selector = gdt::GDT_CODE;
        this->IST = ist;
        this->TypeAttr = typeattr;
        this->Offset2 = static_cast<uint16_t>(israddr >> 16);
        this->Offset3 = static_cast<uint32_t>(israddr >> 32);
        this->Zero = 0;
    }

    static const char *exception_messages[32]
    {
        "Division by zero",
        "Debug",
        "Non-maskable interrupt",
        "Breakpoint",
        "Detected overflow",
        "Out-of-bounds",
        "Invalid opcode",
        "No coprocessor",
        "Double fault",
        "Coprocessor segment overrun",
        "Bad TSS",
        "Segment not present",
        "Stack fault",
        "General protection fault",
        "Page fault",
        "Unknown interrupt",
        "Coprocessor fault",
        "Alignment check",
        "Machine check",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
    };

    static void exception_handler(cpu::registers_t *regs)
    {
        lockit(lock);

        log::println();
        log::errorln("Exception: {} on CPU {}", exception_messages[regs->int_no], (smp::initialised ? this_cpu()->id : 0));
        log::errorln("Address: 0x{:X}", regs->rip);

        if (regs->int_no == 8 || (regs->int_no >= 10 && regs->int_no <= 14) || regs->int_no == 17 || regs->int_no == 21 || regs->int_no == 29 || regs->int_no == 30)
            log::errorln("Error code: 0b{:b}", regs->error_code);

        if (smp::initialised == true)
            this_cpu()->lapic.ipi(panic_int | (0b11 << 18), 0);

        trace::print(regs->rbp, regs->rip, log::error_prefix);

        log::errorln("System halted!\n");
        arch::halt(false);
    }

    static void eoi(uint64_t int_no)
    {
        if (ioapic::initialised == true)
            this_cpu()->lapic.eoi();
        else
            pic::eoi(int_no);
    }

    extern "C" void int_handler(cpu::registers_t *regs)
    {
        if (regs->int_no < 32)
            exception_handler(regs);
        else if (regs->int_no >= 32 && regs->int_no < 256)
        {
            auto &handler = handlers[regs->int_no];

            if (handler.eoi_first == true)
                eoi(regs->int_no);

            handlers[regs->int_no](regs);

            if (handler.eoi_first == false)
                eoi(regs->int_no);
        }
        else PANIC("Unknown interrupt!");
    }

    extern "C" void *int_table[];
    void init()
    {
        log::infoln("Initialising IDT...");

        idtr.Limit = sizeof(IDTEntry) * 256 - 1;
        idtr.Base = reinterpret_cast<uint64_t>(&idt);

        for (size_t i = 0; i < 256; i++)
            idt[i].set(int_table[i]);

        idt[INT_SYSCALL].set(int_table[INT_SYSCALL], 0xEE);
        handlers[INT_SYSCALL].reserve();

        auto [handler, vector] = allocate_handler(IRQ(16));
        handler.set([](cpu::registers_t *)
        {
            arch::halt(false);
        });
        panic_int = vector;
    }
} // namespace idt