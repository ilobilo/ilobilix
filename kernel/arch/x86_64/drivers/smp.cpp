// Copyright (C) 2022  ilobilo

#include <arch/x86_64/drivers/timers/tsc.hpp>
#include <arch/x86_64/cpu/lapic.hpp>
#include <arch/x86_64/cpu/gdt.hpp>
#include <arch/x86_64/cpu/idt.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <drivers/smp.hpp>
#include <init/kernel.hpp>
#include <lib/panic.hpp>
#include <lib/log.hpp>
#include <mm/vmm.hpp>

namespace smp
{
    extern "C" cpu_t *this_cpu()
    {
        return &cpus[read_gs(0)];
    }

    void cpu_bsp_init(limine_smp_info *cpu)
    {
        auto cpuptr = reinterpret_cast<cpu_t*>(cpu->extra_argument);

        gdt::init(cpuptr->id);
        idt::init();
        idt::idtr.load();

        cpu::set_kernel_gs(cpu->extra_argument);
        cpu::set_gs(cpu->extra_argument);

        this_cpu()->lapic.init();
    }

    void cpu_init(limine_smp_info *cpu)
    {
        auto cpuptr = reinterpret_cast<cpu_t*>(cpu->extra_argument);

        if (cpuptr->arch_id != bsp_id)
        {
            cpu::enablePAT();
            vmm::kernel_pagemap->load();

            gdt::init(cpuptr->id);
            idt::idtr.load();

            cpu::set_kernel_gs(cpu->extra_argument);
            cpu::set_gs(cpu->extra_argument);
        }

        cpu::enableSSE();
        cpu::enableSMEP();
        cpu::enableSMAP();
        cpu::enableUMIP();

        uint32_t a = 0, b = 0, c = 0, d = 0;
        cpu::id(1, 0, a, b, c, d);
        if (c & 0x08000000)
        {
            write_cr(4, read_cr(4) | (1 << 18));

            assert(cpu::id(0x0D, 0, a, b, c, d), "CPUID failure");
            this_cpu()->fpu_storage_size = c;
            this_cpu()->fpu_restore = cpu::xrstor;

            assert(cpu::id(0x0D, 1, a, b, c, d), "CPUID failure");
            if (a & 0x00000001) this_cpu()->fpu_save = cpu::xsaveopt;
            else this_cpu()->fpu_save = cpu::xsave;
        }
        else if (d & 0x01000000)
        {
            write_cr(4, read_cr(4) | (1 << 9));

            this_cpu()->fpu_storage_size = 512;
            this_cpu()->fpu_save = cpu::fxsave;
            this_cpu()->fpu_restore = cpu::fxrstor;
        }
        else PANIC("No known SIMD save mechanism");

        if (cpuptr->arch_id != bsp_id)
            this_cpu()->lapic.init();

        // timers::tsc::init();
    }
} // namespace smp