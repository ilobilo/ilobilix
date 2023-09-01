// Copyright (C) 2022-2023  ilobilo

#include <drivers/proc.hpp>
#include <drivers/smp.hpp>
#include <init/kernel.hpp>
#include <arch/arch.hpp>
#include <lib/panic.hpp>
#include <lib/log.hpp>
#include <mm/vmm.hpp>

namespace smp
{
    bool initialised = false;
    cpu_t *cpus = nullptr;
    uint64_t bsp_id = 0;

    extern void cpu_bsp_init(limine_smp_info *cpu);
    extern void cpu_init(limine_smp_info *cpu);

    static constexpr auto get_arch_id(limine_smp_info *cpu)
    {
#if defined(__x86_64__)
        return cpu->lapic_id;
#elif defined(__aarch64__)
        return cpu->mpidr;
#else
#  error Unknown architecture
#endif
    }

    void bsp_init()
    {
        cpus = new cpu_t[smp_request.response->cpu_count]();

#if defined(__x86_64__)
        bsp_id = smp_request.response->bsp_lapic_id;
#elif defined (__aarch64__)
        bsp_id = smp_request.response->bsp_mpidr;
#else
#  error Unknown architecture
#endif

        for (size_t i = 0; i < smp_request.response->cpu_count; i++)
        {
            limine_smp_info *smp_info = smp_request.response->cpus[i];
            if (get_arch_id(smp_info) != bsp_id)
                continue;

            smp_info->extra_argument = reinterpret_cast<uint64_t>(&cpus[i]);
            cpus[i].arch_id = get_arch_id(smp_info);
            cpus[i].id = i;

            cpu_bsp_init(smp_info);
        }
    }

    void init()
    {
        log::infoln("SMP: Initialising...");

        auto cpu_entry = [](limine_smp_info *cpu)
        {
            static std::mutex lock;
            {
                std::unique_lock guard(lock);
                cpu_init(cpu);

                log::infoln("SMP: CPU {} is up", this_cpu()->id);
                this_cpu()->is_up = true;
            }

            if (bsp_id != this_cpu()->arch_id)
                proc::init();
        };

        for (size_t i = 0; i < smp_request.response->cpu_count; i++)
        {
            limine_smp_info *smp_info = smp_request.response->cpus[i];
            smp_info->extra_argument = reinterpret_cast<uint64_t>(&cpus[i]);
            cpus[i].arch_id = get_arch_id(smp_info);
            cpus[i].id = i;

            if (bsp_id != cpus[i].arch_id)
            {
                smp_request.response->cpus[i]->goto_address = cpu_entry;
                while (cpus[i].is_up == false)
                    arch::pause();
            }
            else cpu_entry(smp_info);
        }

        initialised = true;
    }
} // namespace smp