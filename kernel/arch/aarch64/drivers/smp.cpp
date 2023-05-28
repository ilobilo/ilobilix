// Copyright (C) 2022-2023  ilobilo

#include <drivers/smp.hpp>
#include <init/kernel.hpp>
#include <cpu/cpu.hpp>
#include <mm/vmm.hpp>

namespace smp
{
    extern "C" cpu_t *this_cpu()
    {
        return reinterpret_cast<cpu_t*>(cpu::get_el1_base());
    }

    void cpu_bsp_init(limine_smp_info *cpu)
    {
        cpu::set_el1_base(cpu->extra_argument);
    }

    void cpu_init(limine_smp_info *cpu)
    {
        auto cpuptr = reinterpret_cast<cpu_t*>(cpu->extra_argument);

        if (cpuptr->arch_id != bsp_id)
        {
            vmm::kernel_pagemap->load();
            cpu::set_el1_base(cpu->extra_argument);
        }
    }
} // namespace smp