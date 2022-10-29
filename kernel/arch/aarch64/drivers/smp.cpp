// Copyright (C) 2022  ilobilo

#include <drivers/smp.hpp>
#include <init/kernel.hpp>
#include <cpu/cpu.hpp>

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
        cpu::set_el1_base(cpu->extra_argument);
    }
} // namespace smp