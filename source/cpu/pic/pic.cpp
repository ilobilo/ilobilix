// Copyright (C) 2022  ilobilo

#include <cpu/pic/pic.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>
#include <main.hpp>

namespace pic
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
        log::info("Initialising PIC...");

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
} // namespace pic