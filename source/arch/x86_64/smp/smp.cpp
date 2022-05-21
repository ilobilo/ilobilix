// Copyright (C) 2022  ilobilo

#if defined(__x86_64__)

#include <arch/x86_64/apic/apic.hpp>
#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/idt/idt.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <mm/vmm/vmm.hpp>
#include <lib/panic.hpp>
#include <lib/log.hpp>
#include <smp/smp.hpp>
#include <main.hpp>
#include <cpuid.h>

namespace arch::x86_64::smp
{
    void cpu_init(limine_smp_info *cpu)
    {
        mm::vmm::kernel_pagemap->switchTo();

        gdt::initcpu(reinterpret_cast<::smp::cpu_t*>(cpu->extra_argument)->id);
        idt::idtr.load();

        cpu::set_kernel_gs(cpu->extra_argument);
        cpu::set_gs(cpu->extra_argument);

        this_cpu->lapic_id = cpu->lapic_id;

        cpu::enableSSE();
        cpu::enableSMEP();
        cpu::enableSMAP();
        cpu::enableUMIP();
        cpu::enablePAT();

        uint32_t a = 0, b = 0, c = 0, d = 0;
        __get_cpuid(1, &a, &b, &c, &d);
        if (c & bit_XSAVE)
        {
            write_cr(4, read_cr(4) | (1 << 18));

            assert(__get_cpuid_count(0x0D, 0, &a, &b, &c, &d), "CPUID failure");
            this_cpu->fpu_storage_size = c;
            this_cpu->fpu_restore = cpu::xrstor;

            assert(__get_cpuid_count(0x0D, 1, &a, &b, &c, &d), "CPUID failure");
            if (a & bit_XSAVEOPT) this_cpu->fpu_save = cpu::xsaveopt;
            else this_cpu->fpu_save = cpu::xsave;
        }
        else if (d & bit_FXSAVE)
        {
            write_cr(4, read_cr(4) | (1 << 9));

            this_cpu->fpu_storage_size = 512;
            this_cpu->fpu_save = cpu::fxsave;
            this_cpu->fpu_restore = cpu::fxrstor;
        }
        else panic("No known SIMD save mechanism");

        if (smp_request.response->bsp_lapic_id != cpu->lapic_id && apic::initialised) apic::lapic::init(this_cpu->lapic_id);
    }
} // namespace arch::x86_64::smp

#endif