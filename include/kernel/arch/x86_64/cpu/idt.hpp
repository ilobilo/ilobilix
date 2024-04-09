// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <lib/interrupts.hpp>
#include <cpu/cpu.hpp>
#include <cstdint>

namespace idt
{
    inline constexpr uint8_t INT_SYSCALL = 0x80;

    inline constexpr uint8_t IRQ(uint8_t num)
    {
        return num + 0x20;
    }

    struct [[gnu::packed]] entry
    {
        uint16_t offset0;
        uint16_t selector;
        uint8_t ist;
        uint8_t typeattr;
        uint16_t offset1;
        uint32_t offset2;
        uint32_t zero;

        void set(void *isr, uint8_t typeattr = 0x8E, uint8_t ist = 0);
    };

    struct [[gnu::packed]] ptr
    {
        uint16_t limit;
        uint64_t base;

        void load() const
        {
            asm volatile ("cli");
            asm volatile ("lidt %0" :: "memory"(*this));
            asm volatile ("sti");
        }
    };
    inline constexpr ptr invalid { 0, 0 };

    extern interrupts::handler handlers[];
    extern uint8_t panic_int;
    extern entry idt[];
    extern ptr idtr;

    std::pair<interrupts::handler &, uint8_t> allocate_handler(uint8_t hint = IRQ(16));

    void mask(uint8_t irq);
    void unmask(uint8_t irq);

    void init();
} // namespace idt