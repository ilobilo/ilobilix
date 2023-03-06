// Copyright (C) 2022-2023  ilobilo

#include <drivers/proc.hpp>
#include <drivers/smp.hpp>
#include <lib/misc.hpp>
#include <mm/pmm.hpp>

namespace proc
{
    // TODO

    extern "C" thread *this_thread()
    {
        return nullptr;
    }

    void thread_finalise(thread *thread, uintptr_t pc, uintptr_t arg)
    {
        // auto proc = thread->parent;

        if (thread->user == true) { }
        else
        {
            uintptr_t pstack = pmm::alloc<uintptr_t>(default_stack_size / pmm::page_size);
            thread->stack = tohh(pstack) + default_stack_size;

            thread->stacks.push_back(pstack);
        }
    }

    void thread_delete(thread *thread)
    {
    }

    void save_thread(thread *thread, cpu::registers_t *regs)
    {
        thread->regs = *regs;

        thread->el0_base = cpu::get_el0_base();
    }

    void load_thread(thread *thread, cpu::registers_t *regs)
    {
        thread->running_on = this_cpu()->id;
        thread->parent->pagemap->load();

        cpu::set_el1_base(reinterpret_cast<uint64_t>(thread));
        cpu::set_el0_base(thread->el0_base);

        *regs = thread->regs;
    }

    void wake_up(size_t id, bool everyone)
    {
    }

    void reschedule(uint64_t ms)
    {
    }

    void arch_init(void (*func)(cpu::registers_t *regs))
    {
    }
} // namespace proc