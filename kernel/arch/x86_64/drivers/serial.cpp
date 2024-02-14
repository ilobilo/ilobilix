// Copyright (C) 2022-2024  ilobilo

#include <arch/x86_64/lib/io.hpp>
#include <arch/arch.hpp>

namespace serial
{
    enum COMS
    {
        COM1 = 0x3F8,
        COM2 = 0x2F8,
        COM3 = 0x3E8,
        COM4 = 0x2E8
    };

    static void printc(char c, COMS com)
    {
        while (not (io::in<uint8_t>(com + 5) & (1 << 5)))
            arch::pause();

        io::out<uint8_t>(com, c);
    }
    void printc(char c) { printc(c, COM1); }

    static char readc(COMS com)
    {
        while (not (io::in<uint8_t>(com + 5) & (1 << 0)))
            arch::pause();

        return io::in<uint8_t>(com);
    }
    char readc() { return readc(COM1); }

    static void initport(COMS com)
    {
        io::out<uint8_t>(com + 1, 0x00);
        io::out<uint8_t>(com + 3, 0x80);
        io::out<uint8_t>(com + 0, 0x01);
        io::out<uint8_t>(com + 1, 0x00);
        io::out<uint8_t>(com + 3, 0x03);
        io::out<uint8_t>(com + 2, 0xC7);
        io::out<uint8_t>(com + 4, 0x0B);
    }

    void early_init()
    {
        initport(COM1);
        initport(COM2);

        // io::out<uint8_t>(COM1 + 1, 0x01);
        // io::out<uint8_t>(COM2 + 1, 0x01);
    }

    // unused on x86_64
    void second_early_init() { }
    void init() { }
} // namespace serial