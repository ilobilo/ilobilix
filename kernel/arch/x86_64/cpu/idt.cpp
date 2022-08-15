// Copyright (C) 2022  ilobilo

#include <arch/x86_64/cpu/ioapic.hpp>
#include <arch/x86_64/cpu/gdt.hpp>
#include <arch/x86_64/cpu/idt.hpp>
#include <arch/x86_64/cpu/pic.hpp>
#include <kernel/kernel.hpp>
#include <drivers/term.hpp>
#include <drivers/acpi.hpp>
#include <drivers/smp.hpp>
#include <lib/panic.hpp>
#include <lib/trace.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>

namespace idt
{
    int_handler_t handlers[256];
    static lock_t lock;
    IDTEntry idt[256];
    IDTPtr idtr;

    std::pair<int_handler_t&, uint8_t> allocate_handler(uint8_t hint)
    {
        if (acpi::madthdr->legacy_pic() == true)
        {
            if ((hint >= IRQ(0) && hint <= IRQ(15)) && handlers[hint].get() == false)
                return std::pair<int_handler_t&, uint8_t>(handlers[hint], hint);
        }

        for (size_t i = IRQ(0); i < 256; i++)
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
        log::error("Exception: %s on CPU %zu", exception_messages[regs->int_no], (smp::initialised ? this_cpu()->id : 0));
        log::error("Address: 0x%lX", regs->rip);

        if (regs->int_no == 8 || (regs->int_no >= 10 && regs->int_no <= 14) || regs->int_no == 17 || regs->int_no == 21 || regs->int_no == 29 || regs->int_no == 30)
            log::error("Error code: 0b%b", regs->error_code);

        if (smp::initialised == true)
            this_cpu()->lapic.ipi(INT_PANIC | (0b10 << 18), 0);

        trace::print(regs->rbp, log::error);

        log::error("System halted!\n");
        while (true) asm volatile ("cli; hlt");
        __builtin_unreachable();
    }

    extern "C" void int_handler(cpu::registers_t *regs)
    {
        if (regs->int_no < 32) exception_handler(regs);
        else if (regs->int_no >= 32 && regs->int_no < 256)
        {
            handlers[regs->int_no](regs);

            if (ioapic::initialised == true)
                this_cpu()->lapic.eoi();
            else
                pic::eoi(regs->int_no);
        }
        else PANIC("Unknown interrupt!");
    }

    extern "C" void *int_table[];
    void init()
    {
        log::info("Initialising IDT...");

        idtr.Limit = sizeof(IDTEntry) * 256 - 1;
        idtr.Base = reinterpret_cast<uint64_t>(&idt);

        for (size_t i = 0; i < 256; i++)
            idt[i].set(int_table[i]);
        idt[INT_SYSCALL].set(int_table[INT_SYSCALL], 0xEE);
    }
} // namespace idt