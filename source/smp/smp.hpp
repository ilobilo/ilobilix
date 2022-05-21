// Copyright (C) 2022  ilobilo

#if defined(__x86_64__)
#include <arch/x86_64/cpu/cpu.hpp>
#elif defined(__aarch64__)
#include <arch/arm64/cpu/cpu.hpp>
#endif
#include <lib/errno.hpp>
#include <cstddef>

namespace smp
{
    struct cpu_t
    {
        uint64_t id;

        #if defined(__x86_64__)
        uint32_t lapic_id = 0;
        uint64_t lapic_ticks_in_1ms = 0;

        uint64_t fpu_storage_size = 512;
        void (*fpu_save)(uint8_t*);
        void (*fpu_restore)(uint8_t*);
        #elif defined(__aarch64__)
        #endif

        errno_t err;
        volatile bool is_up;
    };

    extern bool initialised;
    extern cpu_t *cpus;

    #if defined(__x86_64__)
    #define this_cpu ({ &::smp::cpus[read_gs(0)]; })
    #elif defined(__aarch64__)
    #define this_cpu ({ &::smp::cpus[*reinterpret_cast<uint64_t*>(arch::arm64::cpu::get_base())]; })
    #endif

    void init();
} // namespace smp