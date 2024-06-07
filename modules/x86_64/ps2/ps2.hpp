// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <cstdint>

namespace ps2
{
    uint8_t read();
    void write(uint16_t port, uint8_t data);
} // namespace ps2