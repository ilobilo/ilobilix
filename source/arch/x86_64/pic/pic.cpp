// Copyright (C) 2022  ilobilo

#if defined(__x86_64__) || defined(_M_X64)

#include <arch/x86_64/pic/pic.hpp>
#include <arch/x86_64/misc/io.hpp>
#include <main.hpp>

namespace arch::x86_64::pic
{
    void eoi(uint64_t int_no)
    {
        if (int_no >= 40) outb(PIC2_COMMAND, PIC_EOI);
        outb(PIC1_COMMAND, PIC_EOI);
    }

    void disable()
    {
        outb(0xA1, 0xFF);
        outb(0x21, 0xFF);
    }

    void init()
    {
        outb(0x20, 0x11);
        outb(0xA0, 0x11);
        outb(0x21, 0x20);
        outb(0xA1, 0x28);
        outb(0x21, 0x04);
        outb(0xA1, 0x02);
        outb(0x21, 0x01);
        outb(0xA1, 0x01);
        outb(0x21, 0x00);
        outb(0xA1, 0x00);
    }
} // namespace arch::x86_64::pic

#endif