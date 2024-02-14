// Copyright (C) 2022-2024  ilobilo

// #include <arch/x86_64/drivers/timers/tsc.hpp>
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
        return &cpus[rdreg(gs:0)];
    }

    void cpu_bsp_init(limine_smp_info *cpu)
    {
        auto cpuptr = reinterpret_cast<cpu_t*>(cpu->extra_argument);

        gdt::init(cpuptr->id);
        idt::init();
        idt::idtr.load();

        cpu::set_kernel_gs(cpu->extra_argument);
        cpu::set_gs(cpu->extra_argument);

        cpuptr->lapic.init();
    }

    uint64_t cpu_t::fpu_storage_size = 512;

    extern "C" void syscall_entry();
    void cpu_init(limine_smp_info *cpu)
    {
        auto cpuptr = reinterpret_cast<cpu_t*>(cpu->extra_argument);

        if (cpuptr->arch_id != bsp_id)
        {
            cpu::enablePAT();
            vmm::kernel_pagemap->load(true);

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
            wrreg(cr4, rdreg(cr4) | (1 << 18));

            assert(cpu::id(0x0D, 0, a, b, c, d), "CPUID failure");
            cpuptr->fpu_storage_size = c;
            cpuptr->fpu_restore = cpu::xrstor;

            assert(cpu::id(0x0D, 1, a, b, c, d), "CPUID failure");
            if (a & 0x00000001)
                cpuptr->fpu_save = cpu::xsaveopt;
            else
                cpuptr->fpu_save = cpu::xsave;
        }
        else if (d & 0x01000000)
        {
            wrreg(cr4, rdreg(cr4) | (1 << 9));

            cpuptr->fpu_storage_size = 512;
            cpuptr->fpu_save = cpu::fxsave;
            cpuptr->fpu_restore = cpu::fxrstor;
        }
        else PANIC("No known SIMD save mechanism!");

        cpu::wrmsr(0xC0000080, cpu::rdmsr(0xC0000080) | (1 << 0)); // IA32_EFER enable syscall
        cpu::wrmsr(0xC0000081, ((uint64_t(gdt::GDT_DATA) | 0x03) << 48) | (uint64_t(gdt::GDT_CODE) << 32)); // IA32_STAR ss and cs
        cpu::wrmsr(0xC0000082, reinterpret_cast<uint64_t>(syscall_entry)); // IA32_LSTAR handler
        cpu::wrmsr(0xC0000084, ~uint32_t(2)); // IA32_FMASK rflags mask

        if (cpuptr->arch_id != bsp_id)
            cpuptr->lapic.init();

        // timers::tsc::init();
    }
} // namespace smp