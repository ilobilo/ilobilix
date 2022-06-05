// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/errno.hpp>
#include <cpu/cpu.hpp>
#include <cstdint>

namespace smp
{
    struct cpu_t
    {
        uint64_t id;

        uint32_t lapic_id = 0;
        uint64_t lapic_ticks_in_1ms = 0;

        uint64_t fpu_storage_size = 512;
        void (*fpu_save)(uint8_t*);
        void (*fpu_restore)(uint8_t*);

        errno_t err;
        volatile bool is_up;
    };

    extern bool initialised;
    extern cpu_t *cpus;

    #define this_cpu ({ &::smp::cpus[read_gs(0)]; })

    void init();
} // namespace smp