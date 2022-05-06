// Copyright (C) 2022  ilobilo

#include <drivers/serial/serial.hpp>
#include <drivers/term/term.hpp>
#include <lib/lock.hpp>
#include <lib/io.hpp>
#include <cstdarg>

namespace serial
{
    static lock_t lock;

    static bool is_transmit_empty(COMS com = COM1)
    {
        return inb(com + 5) & 0x20;
    }

    // static bool received(COMS com = COM1)
    // {
    //     uint8_t status = inb(com + 5);
    //     return (status != 0xFF) && (status & 1);
    // }

    // static char read(COMS com = COM1)
    // {
    //     while (!received());
    //     return inb(com);
    // }

    void printc(char c, void *arg)
    {
        while (!is_transmit_empty());
        outb(reinterpret_cast<uintptr_t>(arg), c);
    }

    int print(COMS com, const char *fmt, ...)
    {
        lockit(lock);

        va_list args;
        va_start(args, fmt);
        int ret = vfctprintf(printc, reinterpret_cast<void*>(com), fmt, args);
        va_end(args);

        return ret;
    }

    static void initport(COMS com)
    {
        outb(com + 1, 0x00);
        outb(com + 3, 0x80);
        outb(com + 0, 0x01);
        outb(com + 1, 0x00);
        outb(com + 3, 0x03);
        outb(com + 2, 0xC7);
        outb(com + 4, 0x0B);

        print(com, "\033[0m\n");
    }

    void init()
    {
        initport(COM1);
        initport(COM2);

        outb(COM1 + 1, 0x01);
        outb(COM2 + 1, 0x01);
    }
} // namespace serial