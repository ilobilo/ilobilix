// Copyright (C) 2022-2024  ilobilo

#include <arch/x86_64/cpu/gdt.hpp>
#include <arch/x86_64/cpu/idt.hpp>

#include <drivers/proc.hpp>
#include <drivers/smp.hpp>

#include <init/kernel.hpp>
#include <lib/misc.hpp>
#include <mm/pmm.hpp>

namespace proc
{
    static uint8_t sched_vector = 0;

    extern "C" thread *this_thread()
    {
        return reinterpret_cast<thread*>(rdreg(gs:8));
    }

    void thread_finalise(thread *thread, uintptr_t pc, uintptr_t arg)
    {
        // auto proc = thread->parent;

        thread->regs.rflags = 0x202;
        thread->regs.rip = pc;
        thread->regs.rdi = arg;

        thread->fpu_storage_pages = div_roundup(this_cpu()->fpu_storage_size, pmm::page_size);
        thread->fpu_storage = tohh(pmm::alloc<uint8_t*>(thread->fpu_storage_pages));

        uintptr_t pkstack = pmm::alloc<uintptr_t>(kernel_stack_size / pmm::page_size);
        thread->kstack = tohh(pkstack) + kernel_stack_size;
        thread->stacks.push_back(std::make_pair(pkstack, kernel_stack_size));

        uintptr_t ppfstack = pmm::alloc<uintptr_t>(kernel_stack_size / pmm::page_size);
        thread->pfstack = tohh(ppfstack) + kernel_stack_size;
        thread->stacks.push_back(std::make_pair(ppfstack, kernel_stack_size));

        thread->gs_base = reinterpret_cast<uintptr_t>(thread);

        if (thread->user == true)
        {
            thread->regs.cs = gdt::GDT_USER_CODE | 0x03;
            thread->regs.ss = gdt::GDT_USER_DATA | 0x03;

            thread->regs.rsp = thread->stack;

            this_cpu()->fpu_restore(thread->fpu_storage);

            uint16_t default_fcw = 0b1100111111;
            asm volatile ("fldcw %0" :: "m"(default_fcw) : "memory");
            uint32_t default_mxcsr = 0b1111110000000;
            asm volatile ("ldmxcsr %0" :: "m"(default_mxcsr) : "memory");

            this_cpu()->fpu_save(thread->fpu_storage);
        }
        else
        {
            thread->regs.cs = gdt::GDT_CODE;
            thread->regs.ss = gdt::GDT_DATA;

            thread->regs.rsp = thread->stack = thread->kstack;
        }
    }

    void thread_delete(thread *thread)
    {
        pmm::free(fromhh(thread->fpu_storage), thread->fpu_storage_pages);
    }

    void save_thread(thread *thread, cpu::registers_t *regs)
    {
        thread->regs = *regs;

        thread->gs_base = cpu::get_kernel_gs();
        thread->fs_base = cpu::get_fs();

        this_cpu()->fpu_save(thread->fpu_storage);
    }

    void load_thread(thread *thread, cpu::registers_t *regs)
    {
        thread->running_on = this_cpu()->id;

        gdt::tsses[this_cpu()->id].ist[1] = thread->pfstack;
        this_cpu()->fpu_restore(thread->fpu_storage);
        thread->parent->pagemap->load(false);

        cpu::set_gs(reinterpret_cast<uint64_t>(thread));
        cpu::set_kernel_gs(thread->gs_base);
        cpu::set_fs(thread->fs_base);

        *regs = thread->regs;
    }

    void wake_up(size_t id, bool everyone)
    {
        if (everyone == true)
            this_cpu()->lapic.ipi(sched_vector | (0b10 << 18), 0);
        else
            this_cpu()->lapic.ipi(sched_vector, smp::cpus[id].lapic.id);
    }

    void reschedule(uint64_t ms)
    {
        // if (ms == 0)
        //     this_cpu()->lapic.ipi(sched_vector, this_cpu()->lapic.id);
        // else
            this_cpu()->lapic.timer(sched_vector, ms, lapic::timermodes::oneshot);
    }

    void arch_init(void (*func)(cpu::registers_t *regs))
    {
        gdt::tsses[this_cpu()->id].ist[0] = tohh(pmm::alloc<uint64_t>(kernel_stack_size / pmm::page_size)) + kernel_stack_size;
        idt::idt[14].ist = 2;

        [[maybe_unused]]
        static auto once = [func]() -> bool
        {
            auto [handler, vector] = idt::allocate_handler();
            handler.set([func](cpu::registers_t *regs)
            {
                func(regs);
            });

            handler.eoi_first = true;
            idt::idt[vector].ist = 1;
            sched_vector = vector;

            return true;
        }();
    }
} // namespace proc