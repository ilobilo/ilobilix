// Copyright (C) 2022  ilobilo

#if defined(__x86_64__)
#include <arch/x86_64/smp/smp.hpp>
#elif defined(__aarch64__)
#include <arch/arm64/smp/smp.hpp>
#endif
#include <lib/alloc.hpp>
#include <lib/log.hpp>
#include <smp/smp.hpp>

namespace smp
{
    bool initialised = false;
    cpu_t *cpus = nullptr;
    static lock_t lock;

    static void cpu_init(limine_smp_info *cpu)
    {
        lock.lock();

        #if defined(__x86_64__)
        arch::x86_64::smp::cpu_init(cpu);
        #elif defined(__aarch64__)
        arch::arm64::smp::cpu_init(cpu);
        #endif

        log::info("CPU %ld is up", this_cpu->id);
        this_cpu->is_up = true;

        lock.unlock();
        if (smp_request.response->bsp_lapic_id != cpu->lapic_id)
        {
            while (true)
            {
                #if defined(__x86_64__)
                asm volatile ("hlt");
                #endif
            }
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