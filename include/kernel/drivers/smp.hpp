// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/types.hpp>
#include <cstddef>
#include <cstdint>

#if defined(__x86_64__)
#include <arch/x86_64/cpu/lapic.hpp>
#endif

namespace smp
{
    struct cpu_t
    {
        size_t id = 0;

        #if defined(__x86_64__)

        uint32_t arch_id = 0;

        lapic::lapic lapic;

        uint64_t lapic_ticks_per_ms = 0;
        uint64_t tsc_ticks_per_ns = 0;

        uint64_t fpu_storage_size = 512;
        void (*fpu_save)(uint8_t*);
        void (*fpu_restore)(uint8_t*);

        #endif

        errno_t err;
        volatile bool is_up = false;
    };

    extern bool initialised;
    extern cpu_t *cpus;
    extern uint64_t bsp_id;

    void init();
    void late_init();
} // namespace smp

extern "C" smp::cpu_t *this_cpu();