// Copyright (C) 2022  ilobilo

#if defined(__x86_64__) || defined(_M_X64)

#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/idt/idt.hpp>
#include <arch/x86_64/pic/pic.hpp>
#include <drivers/term/term.hpp>
#include <lib/panic.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <main.hpp>

namespace arch::x86_64::idt
{
    int_handler handlers[256];
    static lock_t lock;
    IDTEntry idt[256];
    IDTPtr idtr;

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

        static bool halt = true;

        log::println();
        log::error("System exception!\n");
        // log::error("Exception: %s on CPU %zu\n", exception_messages[regs->int_no], (smp::initialised ? this_cpu->id : 0));
        log::error("Exception: %s\n", exception_messages[regs->int_no]);
        log::error("Address: 0x%lX\n", regs->rip);
        log::error("Error code: 0x%lX, 0b%b\n", regs->error_code, regs->error_code);

        switch (regs->int_no)
        {
        }

        if (!halt)
        {
            log::print("\n");
            return;
        }

        printf("\n\n[\033[31mPANIC\033[0m] System Exception!\n");
        // printf("[\033[31mPANIC\033[0m] Exception: %s on CPU %zu\n", exception_messages[regs->int_no], (smp::initialised ? this_cpu->id : 0));
        printf("[\033[31mPANIC\033[0m] Exception: %s\n", exception_messages[regs->int_no]);
        printf("[\033[31mPANIC\033[0m] Address: 0x%lX\n", regs->rip);

        switch (regs->int_no)
        {
            case 8:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
                printf("[\033[31mPANIC\033[0m] Error code: 0x%lX\n", regs->error_code);
                break;
        }

        printf("[\033[31mPANIC\033[0m] System halted!");
        log::error("System halted!\n\n");
        while (true) asm volatile ("cli; hlt");
    }

    static void irq_handler(cpu::registers_t *regs)
    {
        handlers[regs->int_no](regs);
        pic::eoi(regs->int_no);
    }

    extern "C" void int_handler(cpu::registers_t *regs)
    {
        if (regs->int_no < 32) exception_handler(regs);
        else if (regs->int_no >= 32 && regs->int_no < 256) irq_handler(regs);
        else panic("Unknown interrupt!");
    }

    extern "C" void *int_table[];
    void init()
    {
        idtr.Limit = sizeof(IDTEntry) * 256 - 1;
        idtr.Base = reinterpret_cast<uint64_t>(&idt);

        for (size_t i = 0; i < 256; i++) idt[i].set(int_table[i]);
        idt[SYSCALL].set(int_table[SYSCALL], 0xEE);

        idtr.load();
    }
} // namespace arch::x86_64::idt

#endif