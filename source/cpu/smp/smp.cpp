// Copyright (C) 2022  ilobilo

#include <cpu/apic/apic.hpp>
#include <cpu/gdt/gdt.hpp>
#include <cpu/idt/idt.hpp>
#include <cpu/smp/smp.hpp>
#include <mm/vmm/vmm.hpp>
#include <lib/panic.hpp>
#include <lib/log.hpp>
#include <cpu/cpu.hpp>
#include <main.hpp>
#include <cpuid.h>

namespace smp
{
    bool initialised = false;
    cpu_t *cpus = nullptr;
    static lock_t lock;

    void cpu_init(limine_smp_info *cpu)
    {
        lock.lock();

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

        log::info("CPU %ld is up", this_cpu->id);
        this_cpu->is_up = true;

        lock.unlock();
        if (smp_request.response->bsp_lapic_id != cpu->lapic_id)
        {
            if (apic::initialised == true) apic::lapic::init(this_cpu->lapic_id);
            while (true) asm volatile ("hlt");
        }
    }

    void init()
    {
        log::info("Initialising SMP...");

        cpus = new cpu_t[smp_request.response->cpu_count]();

        for (size_t i = 0; i < smp_request.response->cpu_count; i++)
        {
            limine_smp_info *smp_info = smp_request.response->cpus[i];
            smp_info->extra_argument = reinterpret_cast<uint64_t>(&cpus[i]);
            cpus[i].id = i;

            if (smp_request.response->bsp_lapic_id != smp_info->lapic_id)
            {
                smp_request.response->cpus[i]->goto_address = cpu_init;
                while (cpus[i].is_up == false);
            }
            else cpu_init(smp_info);
        }

        initialised = true;
    }
} // namespace smp