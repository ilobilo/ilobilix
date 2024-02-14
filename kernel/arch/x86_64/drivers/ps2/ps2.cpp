// Copyright (C) 2022-2024  ilobilo

#include <arch/x86_64/drivers/ps2/ps2.hpp>
#include <arch/x86_64/drivers/ps2/kbd.hpp>
#include <arch/arch.hpp>
#include <lib/io.hpp>

namespace ps2
{
    uint8_t read()
    {
        while ((io::in<uint8_t>(ports::status) & (1 << 0)) == 0)
            arch::pause();
        return io::in<uint8_t>(ports::data);
    }

    void write(uint16_t port, uint8_t data)
    {
        while ((io::in<uint8_t>(ports::status) & (1 << 1)) != 0)
            arch::pause();
        io::out<uint8_t>(port, data);
    }

    void flush()
    {
        while ((io::in<uint8_t>(ports::status) & (1 << 0)) != 0)
            io::in<uint8_t>(ports::data);
    }

    void init()
    {
        write(ports::command, 0xAD);
        write(ports::command, 0xA7);

        write(ports::command, 0x20);
        auto config = read();

        config |= first_irq_mask | translation;
        if (config & second_clock)
            config |= second_irq_mask;

        write(ports::command, 0x60);
        write(ports::data, config);

        write(ports::command, 0xAE);
        if (config & second_clock)
            write(ports::command, 0xA8);

        kbd::init();

        flush();
    }
} // namespace ps2