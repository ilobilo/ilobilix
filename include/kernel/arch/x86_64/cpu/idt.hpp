// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <lib/interrupts.hpp>
#include <cpu/cpu.hpp>
#include <cstdint>

namespace idt
{
    static constexpr uint8_t INT_SYSCALL = 0x80;

    constexpr inline uint8_t IRQ(uint8_t num)
    {
        return num + 0x20;
    }

    struct [[gnu::packed]] IDTEntry
    {
        uint16_t Offset1;
        uint16_t Selector;
        uint8_t IST;
        uint8_t TypeAttr;
        uint16_t Offset2;
        uint32_t Offset3;
        uint32_t Zero;

        void set(void *isr, uint8_t typeattr = 0x8E, uint8_t ist = 0);
    };

    struct [[gnu::packed]] IDTPtr
    {
        uint16_t Limit;
        uint64_t Base;

        void load()
        {
            asm volatile ("cli");
            asm volatile ("lidt %0" :: "memory"(*this));
            asm volatile ("sti");
        }
    };

    extern interrupts::handler handlers[];
    extern uint8_t panic_int;
    extern IDTEntry idt[];
    extern IDTPtr idtr;

    extern IDTPtr invalid;

    std::pair<interrupts::handler&, uint8_t> allocate_handler(uint8_t hint = IRQ(0));

    void mask(uint8_t irq);
    void unmask(uint8_t irq);

    void init();
} // namespace idt