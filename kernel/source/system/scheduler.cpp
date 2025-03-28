// Copyright (C) 2024-2025  ilobilo

module system.scheduler;

import system.cpu.self;
import system.memory;
import system.time;
import boot;
import arch;
import lib;
import std;

namespace sched
{
    namespace arch
    {
        void init();
        void reschedule(std::size_t ms);

        void finalise(std::shared_ptr<thread> thread, std::uintptr_t ip);
        void deinitialise(std::shared_ptr<thread> thread);

        void save(std::shared_ptr<thread> thread);
        void load(std::shared_ptr<thread> thread);
    } // namespace arch

    namespace
    {
        lib::map::flat_hash<std::size_t, std::shared_ptr<process>> processes;
        lib::spinlock<false> process_lock;

        std::atomic_size_t next_pid = 0;
        std::size_t alloc_pid(std::shared_ptr<process> proc)
        {
            std::unique_lock _ { process_lock };
            auto pid = next_pid++;
            processes[pid] = proc;
            return pid;
        }

        std::shared_ptr<thread> next_thread(bool idle = false)
        {
            auto &self = cpu::self()->sched;
            std::unique_lock _ { self.lock };

            auto &queue = self.queue;
            if (queue.empty() || idle)
                return self.idle_thread;

            auto thread = queue.front();
            queue.pop_front();
            return thread;
        }

        void save(std::shared_ptr<thread> thread, cpu::registers *regs)
        {
            std::memcpy(&thread->regs, regs, sizeof(cpu::registers));
            arch::save(thread);
        }

        void load(std::shared_ptr<thread> thread, cpu::registers *regs)
        {
            std::memcpy(regs, &thread->regs, sizeof(cpu::registers));
            arch::load(thread);
            thread->proc.lock()->pagemap->load();
        }

        void idle()
        {
            ::arch::halt(true);
        }
    } // namespace

    std::uintptr_t thread::allocate_ustack()
    {
        auto parent = proc.lock();
        auto paddr = pmm::alloc<std::uintptr_t>(lib::div_roundup(boot::user_stack_size, pmm::page_size));
        auto vaddr = (parent->next_stack_top -= boot::user_stack_size);

        auto psize = vmm::pagemap::max_page_size(boot::user_stack_size);
        if (!parent->pagemap->map(vaddr, paddr, boot::user_stack_size, vmm::flag::rw, psize))
            lib::panic("could not map user stack");

        stacks.push_back(reinterpret_cast<std::byte *>(paddr));
        return vaddr + boot::user_stack_size;
    }

    std::uintptr_t thread::allocate_kstack()
    {
        auto paddr = pmm::alloc<std::byte *>(lib::div_roundup(boot::kernel_stack_size, pmm::page_size));
        auto vaddr = lib::tohh(paddr);
        std::memset(vaddr, 0, boot::kernel_stack_size);

        stacks.push_back(paddr);
        return reinterpret_cast<std::uintptr_t>(vaddr) + boot::kernel_stack_size;
    }

    std::shared_ptr<thread> thread::create(std::shared_ptr<process> parent, std::uintptr_t ip)
    {
        auto thread = std::make_shared<sched::thread>();

        thread->tid = parent->next_tid++;
        thread->proc = parent;
        thread->status = status::not_ready;
        thread->is_user = false;

        auto stack = thread->allocate_kstack();
        thread->kstack_top = thread->ustack_top = stack;
        arch::finalise(thread, ip);

        std::unique_lock _ { parent->lock };
        parent->threads[thread->tid] = thread;
        // enqueued manually

        return thread;
    }

    void thread::prepare_sleep(std::size_t ms)
    {
        sleep_ints = ::arch::int_switch_status(false);
        sleep_lock.lock();
        status = status::sleeping;

        if (ms)
            sleep_for = ms;
        else
            sleep_for = std::nullopt;
    }

    bool thread::wake_up(std::size_t reason)
    {
        bool ints = ::arch::int_switch_status(false);
        sleep_lock.lock();

        if (status != status::sleeping)
        {
            sleep_lock.unlock();
            ::arch::int_switch(ints);
            return false;
        }

        status = status::ready;
        wake_reason = reason;

        sleep_lock.unlock();
        ::arch::int_switch(ints);
        return true;
    }

    thread::~thread()
    {
        for (auto &stack : stacks)
            pmm::free(stack, lib::div_roundup(boot::kernel_stack_size, pmm::page_size));

        // TODO
    }

    std::shared_ptr<process> process::create(std::shared_ptr<process> parent, std::shared_ptr<vmm::pagemap> pagemap)
    {
        auto proc = std::make_shared<process>();
        proc->pid = alloc_pid(proc);
        proc->pagemap = pagemap;

        if (parent)
        {
            proc->root = parent->root;
            proc->cwd = parent->cwd;
            proc->parent = parent;

            std::unique_lock _ { parent->lock };
            parent->children[proc->pid] = proc;
        }
        else proc->root = proc->cwd = vfs::node::root(true);

        return proc;
    }

    process::~process()
    {
        // TODO
    }

    std::shared_ptr<thread> this_thread()
    {
        return cpu::self()->sched.running_thread;
    }

    std::size_t yield()
    {
        auto thread = this_thread();

        bool eeping = thread->status == status::sleeping;
        bool old = eeping ? thread->sleep_ints : ::arch::int_status();

        if (eeping && thread->sleep_for.has_value())
        {
            auto clock = time::main_clock();
            thread->sleep_until = clock->ns() / 1'000'000 + thread->sleep_for.value();
            thread->sleep_for = std::nullopt;
        }

        arch::reschedule(0);
        ::arch::int_switch(old);
        return eeping ? thread->wake_reason : wake_reason::success;
    }

    std::size_t allocate_cpu()
    {
        std::size_t idx = 0;
        std::size_t min = std::numeric_limits<std::size_t>::max();

        for (std::size_t i = 0; i < cpu::cpu_count; i++)
        {
            auto &sched = cpu::processors[i].sched;
            auto size = sched.queue.size();
            if (size < min)
            {
                min = size;
                idx = i;
            }
        }
        return idx;
    }

    void enqueue(std::shared_ptr<thread> thread, std::size_t cpu_idx)
    {
        if (thread->status == status::running)
            thread->status = status::ready;

        auto &sched = cpu::processors[cpu_idx].sched;
        std::unique_lock _ { sched.lock };
        sched.queue.push_back(thread);
    }

    void schedule(cpu::registers *regs)
    {
        auto self = cpu::self();
        auto &sched = self->sched;

        auto current = sched.running_thread;

        auto first = next_thread();
        auto next = first;
        while (next->status != status::ready)
        {
            if (next->status == status::sleeping)
            {
                auto clock = time::main_clock();
                if (next->sleep_until.has_value() && clock->ns() / 1'000'000 >= next->sleep_until.value())
                {
                    next->status = status::ready;
                    next->wake_reason = wake_reason::success;
                    break;
                }
            }
            enqueue(next, self->idx);
            next = next_thread();
            if (next == first)
            {
                enqueue(next, self->idx);
                next = current != sched.idle_thread ? current : sched.idle_thread;
            }
        }

        if (current && current->status != status::killed)
        {
            save(current, regs);
            if (current != sched.idle_thread)
            {
                if (current->status == status::sleeping)
                    current->sleep_lock.unlock();
                enqueue(current, self->idx);
            }
        }

        sched.running_thread = next;
        next->running_on = reinterpret_cast<decltype(next->running_on)>(self);

        arch::reschedule(fixed_timeslice);
        load(next, regs);
    }

    [[noreturn]] void start()
    {
        static std::atomic_bool should_start = false;

        static auto idle_pagemap = std::make_shared<vmm::pagemap>(vmm::kernel_pagemap.get());
        auto idle_proc = std::make_shared<process>();
        idle_proc->pagemap = idle_pagemap;
        idle_proc->pid = static_cast<std::size_t>(-1);

        auto idle_thread = thread::create(idle_proc, reinterpret_cast<std::uintptr_t>(idle));
        idle_thread->status = status::ready;

        auto self = cpu::self();
        self->sched.idle_proc = idle_proc;
        self->sched.idle_thread = idle_thread;

        arch::init();
        ::arch::int_switch(true);

        if (self->idx == cpu::bsp_idx)
        {
            should_start = true;
            initialised = true;
        }

        while (!should_start)
            ::arch::pause();

        arch::reschedule(0);
        ::arch::halt(true);
    }
} // namespace sched