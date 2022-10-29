// Copyright (C) 2022  ilobilo

#include <arch/arch.hpp>
#include <lib/mmio.hpp>

namespace serial
{
    static uintptr_t uart = 0x9000000;

    void printc(char c)
    {
        while (mmio::in<uint16_t>(uart + 0x18) & (1 << 5))
            arch::pause();

        mmio::out<uint8_t>(uart, c);
    }

    char readc()
    {
        while (mmio::in<uint16_t>(uart + 0x18) & (1 << 4))
            arch::pause();

        return mmio::in<uint8_t>(uart);
    }

    void early_init()
    {
        // Disable the UART.
        mmio::out<uint16_t>(uart + 0x30, 0);

        // Set word length to 8 bits and enable FIFOs
        mmio::out<uint16_t>(uart + 0x2C, (3 << 5) | (1 << 4));

        // Enable UART, TX and RX
        mmio::out<uint16_t>(uart + 0x30, (1 << 0) | (1 << 8) | (1 << 9));
    }

    void init()
    {
    }
} // namespace serial