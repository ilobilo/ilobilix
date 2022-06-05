// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/vector.hpp>
#include <cstdint>
#include <cdi.h>

namespace elf
{
    namespace module
    {
        struct module_t
        {
            uint64_t addr;
            uint64_t size;
            vector<cdi_driver*> drivers;
        };
        extern vector<module_t*> modules;

        bool load(uint64_t address, uint64_t size);
        static inline bool load(auto address, uint64_t size)
        {
            return load(reinterpret_cast<uint64_t>(address), size);
        }
    } // namespace module
    namespace exec
    {
    } // namespace exec
} // namespace elf