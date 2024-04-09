// Copyright (C) 2022-2024  ilobilo

#include <arch/x86_64/cpu/ioapic.hpp>
#include <arch/x86_64/cpu/gdt.hpp>
#include <arch/x86_64/cpu/idt.hpp>
#include <arch/x86_64/cpu/pic.hpp>

#include <drivers/acpi.hpp>
#include <drivers/proc.hpp>
#include <drivers/smp.hpp>

#include <init/kernel.hpp>
#include <arch/arch.hpp>

#include <lib/panic.hpp>
#include <lib/trace.hpp>
#include <lib/log.hpp>

#include <mm/vmm.hpp>

#include <cstdio>

namespace idt
{
    interrupts::handler handlers[256];
    uint8_t panic_int;
    entry idt[256];
    ptr idtr;

    std::pair<interrupts::handler &, uint8_t> allocate_handler(uint8_t hint)
    {
        if (hint < IRQ(0))
            hint += IRQ(0);

        if (acpi::madt::hdr->legacy_pic() == true)
        {
            if ((hint >= IRQ(0) && hint <= IRQ(15)) && handlers[hint].used() == false)
                return { handlers[hint], hint };
        }

        for (size_t i = hint; i < 256; i++)
        {
            if (handlers[i].used() == false && handlers[i].is_reserved() == false)
            {
                handlers[i].reserve();
                return { handlers[i], i };
            }
        }

        PANIC("IDT: Out of interrupt handlers");
    }

    void mask(uint8_t irq)
    {
        if (ioapic::initialised == true && acpi::madt::hdr->legacy_pic())
            ioapic::mask_irq(irq);
        else
            pic::mask(irq);
    }

    void unmask(uint8_t irq)
    {
        if (ioapic::initialised == true && acpi::madt::hdr->legacy_pic())
            ioapic::unmask_irq(irq);
        else
            pic::unmask(irq);
    }

    void entry::set(void *isr, uint8_t typeattr, uint8_t ist)
    {
        uint64_t israddr = reinterpret_cast<uint64_t>(isr);
        this->offset0 = static_cast<uint16_t>(israddr);
        this->selector = gdt::GDT_CODE;
        this->ist = ist;
        this->typeattr = typeattr;
        this->offset1 = static_cast<uint16_t>(israddr >> 16);
        this->offset2 = static_cast<uint32_t>(israddr >> 32);
        this->zero = 0;
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
        if (regs->int_no == 14 && proc::initialised && !(regs->error_code & 0b1))
        {
            auto cr2 = rdreg(cr2);
            if (vmm::page_fault(cr2))
                return;
            // log::errorln("CR2: 0x{:X}", cr2);
        }

        if (regs->cs & 0x03)
            panic(regs, "Exception: {} on CPU {}", exception_messages[regs->int_no], (smp::initialised ? this_cpu()->id : 0));
        else
            panic(regs, regs->rbp, regs->rip, "Exception: {} on CPU {}", exception_messages[regs->int_no], (smp::initialised ? this_cpu()->id : 0));
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

            if (handler.used())
                handlers[regs->int_no](regs);

            if (handler.eoi_first == false)
                eoi(regs->int_no);
        }
        else PANIC("Unknown interrupt {}", regs->int_no);
    }

    extern "C" void *int_table[];
    void init()
    {
        log::infoln("IDT: Initialising...");

        idtr.limit = sizeof(entry) * 256 - 1;
        idtr.base = reinterpret_cast<uint64_t>(&idt);

        for (size_t i = 0; i < 256; i++)
            idt[i].set(int_table[i]);

        idt[INT_SYSCALL].set(int_table[INT_SYSCALL], 0xEE);
        handlers[INT_SYSCALL].reserve();

        auto [handler, vector] = allocate_handler(IRQ(16));
        handler.set([](cpu::registers_t *) { arch::halt(false); });
        panic_int = vector;
    }
} // namespace idt