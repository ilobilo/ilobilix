// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <cstdint>

namespace ps2
{
    enum ports : uint16_t
    {
        data = 0x60,
        status = 0x64,
        command = 0x64
    };

    enum config : uint8_t
    {
        first_irq_mask = (1 << 0),
        second_irq_mask = (1 << 1),
        second_clock = (1 << 5),
        translation = (1 << 6)
    };

    uint8_t read();
    void write(uint16_t port, uint8_t data);
    void flush();

    void init();
} // namespace ps2