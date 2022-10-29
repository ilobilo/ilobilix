// Copyright (C) 2022  ilobilo

#include <arch/x86_64/cpu/pic.hpp>
#include <arch/x86_64/lib/io.hpp>
#include <init/kernel.hpp>
#include <lib/log.hpp>

namespace pic
{
    void eoi(uint64_t int_no)
    {
        if (int_no >= 40) io::out<uint8_t>(PIC2_COMMAND, PIC_EOI);
        io::out<uint8_t>(PIC1_COMMAND, PIC_EOI);
    }

    void mask(uint8_t irq)
    {
        uint16_t port = 0x21;

        if (irq >= 8)
        {
            port = 0xA1;
            irq -= 8;
        }

        io::out<uint8_t>(port, io::in<uint8_t>(port) | (1 << irq));
    }

    void unmask(uint8_t irq)
    {
        uint16_t port = 0x21;

        if (irq >= 8)
        {
            port = 0xA1;
            irq -= 8;
        }

        io::out<uint8_t>(port, io::in<uint8_t>(port) & ~(1 << irq));
    }

    void disable()
    {
        io::out<uint8_t>(0xA1, 0xFF);
        io::out<uint8_t>(0x21, 0xFF);
    }

    void init()
    {
        log::infoln("Initialising PIC...");

        auto a1 = io::in<uint8_t>(0x21);
        auto a2 = io::in<uint8_t>(0xA1);

        io::out<uint8_t>(0x20, 0x11);
        io::out<uint8_t>(0xA0, 0x11);
        io::out<uint8_t>(0x21, 0x20);
        io::out<uint8_t>(0xA1, 0x28);
        io::out<uint8_t>(0x21, 0x04);
        io::out<uint8_t>(0xA1, 0x02);
        io::out<uint8_t>(0x21, 0x01);
        io::out<uint8_t>(0xA1, 0x01);

        io::out<uint8_t>(0x21, a1);
        io::out<uint8_t>(0xA1, a2);
    }
} // namespace pic