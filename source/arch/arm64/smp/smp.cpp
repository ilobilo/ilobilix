// Copyright (C) 2022  ilobilo

#if defined(__aarch64__)

#include <arch/arm64/cpu/cpu.hpp>
#include <mm/vmm/vmm.hpp>
#include <lib/log.hpp>
#include <main.hpp>

namespace arch::arm64::smp
{
    void cpu_init(limine_smp_info *cpu)
    {
        mm::vmm::kernel_pagemap->switchTo();

        cpu::set_base(cpu->extra_argument);
    }
} // namespace arch::arm64::smp

#endif