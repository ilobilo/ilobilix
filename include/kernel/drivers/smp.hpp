// Copyright (C) 2022  ilobilo

#pragma once

#include <cstddef>
#include <cstdint>
#include <cerrno>

#if defined(__x86_64__)
#include <arch/x86_64/cpu/lapic.hpp>
#endif

namespace proc { struct thread; }
namespace smp
{
    struct cpu_t
    {
        // DO NOT MOVE: START
        size_t id = 0;
        void *empty = nullptr;
        // DO NOT MOVE: END

        uint64_t arch_id = 0;

        #if defined(__x86_64__)

        lapic::lapic lapic;

        uint64_t lapic_ticks_per_ms = 0;
        uint64_t tsc_ticks_per_ns = 0;

        uint64_t fpu_storage_size = 512;
        void (*fpu_save)(uint8_t*);
        void (*fpu_restore)(uint8_t*);

        #endif

        proc::thread *idle;

        errno_t error;
        volatile bool is_up = false;
    };

    extern bool initialised;
    extern cpu_t *cpus;
    extern uint64_t bsp_id;

    void bsp_init();
    void init();
} // namespace smp

extern "C" smp::cpu_t *this_cpu();