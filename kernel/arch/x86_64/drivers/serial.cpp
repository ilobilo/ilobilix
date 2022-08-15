// Copyright (C) 2022  ilobilo

#include <arch/x86_64/lib/io.hpp>

namespace serial
{
    enum COMS
    {
        COM1 = 0x3F8,
        COM2 = 0x2F8,
        COM3 = 0x3E8,
        COM4 = 0x2E8
    };

    static bool is_transmit_empty(COMS com = COM1)
    {
        return io::in<uint8_t>(com + 5) & 0x20;
    }

    // static bool received(COMS com = COM1)
    // {
    //     uint8_t status = io::in<uint8_t>(com + 5);
    //     return (status != 0xFF) && (status & 1);
    // }

    // static char read(COMS com = COM1)
    // {
    //     while (!received());
    //     return io::in<uint8_t>(com);
    // }

    static void printc(char c, COMS com)
    {
        while (!is_transmit_empty());
        io::out<uint8_t>(COM1, c);
    }
    void printc(char c) { printc(c, COM1); }

    static void initport(COMS com)
    {
        io::out<uint8_t>(com + 1, 0x00);
        io::out<uint8_t>(com + 3, 0x80);
        io::out<uint8_t>(com + 0, 0x01);
        io::out<uint8_t>(com + 1, 0x00);
        io::out<uint8_t>(com + 3, 0x03);
        io::out<uint8_t>(com + 2, 0xC7);
        io::out<uint8_t>(com + 4, 0x0B);

        auto print = [com](const char *str)
        {
            while (*str++) printc(*(str - 1), com);
        };
        print("\033[0m\n");
    }

    void early_init()
    {
        initport(COM1);
        initport(COM2);

        io::out<uint8_t>(COM1 + 1, 0x01);
        io::out<uint8_t>(COM2 + 1, 0x01);
    }

    void init()
    {
    }
} // namespace serial