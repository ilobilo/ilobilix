// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

namespace elf
{
    namespace module
    {
        bool load(uint64_t address, uint64_t size);
    } // namespace module
    namespace exec
    {
    } // namespace exec
} // namespace elf