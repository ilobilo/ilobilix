// Copyright (C) 2022-2024  ilobilo

#include <lib/containers.hpp>

#include <drivers/proc.hpp>
#include <drivers/smp.hpp>

#include <init/kernel.hpp>
#include <arch/arch.hpp>

#include <mm/pmm.hpp>
#include <deque>

// #include <lib/log.hpp>

namespace proc
{
    std::unordered_map<pid_t, process*> processes;
    static std::mutex lock;

    void thread_finalise(thread *thread, uintptr_t pc, uintptr_t arg);
    void thread_delete(thread *thread);

    void save_thread(thread *thread, cpu::registers_t *regs);
    void load_thread(thread *thread, cpu::registers_t *regs);
    void wake_up(size_t id, bool everyone = false);
    void reschedule(uint64_t ms = 0);

    void arch_init(void (*func)(cpu::registers_t *regs));

    static std::deque<thread*> queues[2];
    static auto active = &queues[0];
    static auto expired = &queues[1];

    thread *next_thread(size_t cpu_id)
    {
        std::unique_lock guard(lock);

        if (active->size() == 0)
            std::swap(active, expired);

        if (active->size() == 0)
            return nullptr;

        for (auto it = active->rbegin(); it != active->rend(); it++)
        {
            auto ret = (*it);
            if (ret->run_on < 0 || ret->run_on == static_cast<ssize_t>(cpu_id))
            {
                active->erase(std::next(it).base());
                ret->in_queue = false;

                return ret;
            }
        }
        return nullptr;
    }

    void yield()
    {
        reschedule();
    }

    void enqueue(thread *thread)
    {
        std::unique_lock guard(lock);
        if (thread->in_queue == true)
            return;

        thread->in_queue = true;
        thread->status = status::ready;
        expired->push_front(thread);
    }

    void dequeue(thread *thread)
    {
        std::unique_lock guard(lock);
        thread->status = status::dequeued;
    }

    void dequeue()
    {
        dequeue(this_thread());
    }

    void unblock(thread *thread)
    {
        std::unique_lock guard(lock);
        if (thread->status != status::blocked)
            return;

        thread->status = status::ready;

        if (thread->in_queue == true)
            return;

        thread->in_queue = true;
        expired->push_front(thread);
    }

    void block(thread *thread)
    {
        lock.lock();
        thread->status = status::blocked;
        lock.unlock();

        if (this_thread() == thread)
            yield();
    }

    void block()
    {
        block(this_thread());
    }

    void exit(thread *thread)
    {
        lock.lock();
        thread->status = status::killed;
        lock.unlock();

        if (this_thread() == thread)
        {
            yield();
            arch::halt();
        }
    }

    [[noreturn]] void exit()
    {
        exit(this_thread());
        std::unreachable();
    }

    void pexit(process *proc, int code)
    {
        assert(proc->pid != 1, "PID 1 exit()");

        arch::int_toggle(false);

        proc->fd_table.reset();

        for (const auto &child : proc->children)
        {
            // child->parent = proc->parent;
            // proc->parent->children.push_back(child);

            // pid 1 (init) inherits the children
            child->parent = processes[1];
            processes[1]->children.push_back(child);
        }
        proc->children.clear();

        for (const auto &zombie : proc->zombies)
        {
            zombie->parent = proc->parent;
            proc->parent->zombies.push_back(zombie);
        }
        proc->zombies.clear();

        erase_from(proc->parent->children, proc);

        // TODO: zombies
        // proc->parent->zombies.push_back(proc);

        proc->status = code;
        proc->exited = true;

        auto me = this_thread();
        for (const auto &thread : proc->threads)
        {
            auto status = thread->status;
            thread->status = status::killed;
            if (thread != me && status == status::running)
                wake_up(thread->running_on);
        }
        // proc->threads.clear(); // threads will remove themselves

        proc->event.trigger();

        free_pid(proc->pid);

        arch::int_toggle(true);
        if (this_thread()->parent == proc)
        {
            vmm::kernel_pagemap->load(false);
            yield();
            std::unreachable();
        }
    }

    [[noreturn]] void pexit(int code)
    {
        pexit(this_thread()->parent, code);
        std::unreachable();
    }

    process::process(std::string_view name) :
        name(name), pagemap(nullptr), next_tid(1), root(vfs::get_root()), cwd(vfs::get_root()), umask(s_iwgrp | s_iwoth),
        fd_table(nullptr), session(nullptr), parent(nullptr), status(0), exited(false), usr_stack_top(def_usr_stack_top)
    {
        this->gid = 0;
        this->sgid = 0;
        this->egid = 0;

        this->uid = 0;
        this->suid = 0;
        this->euid = 0;

        this->pid = alloc_pid(this);
    }

    tid_t process::alloc_tid()
    {
        return this->next_tid++;
    }

    static std::pair<uintptr_t, uintptr_t> map_user_stack(thread *thread, process *parent)
    {
        uintptr_t pstack = pmm::alloc<uintptr_t>(user_stack_size / pmm::page_size);
        uintptr_t vustack = parent->usr_stack_top - user_stack_size;

        assert(parent->pagemap->mmap_range(vustack, pstack, user_stack_size, vmm::mmap::prot_read | vmm::mmap::prot_write, vmm::mmap::map_anonymous), "Could not map user stack");

        parent->usr_stack_top = vustack - pmm::page_size;

        thread->stacks.push_back(std::make_pair(pstack, user_stack_size));
        return { tohh(pstack) + user_stack_size, vustack + user_stack_size };
    }

    thread::thread(process *parent) :
        self(this), error(no_error), parent(parent),
        user(false), in_queue(false), status(status::dequeued), run_on(-1)
    {
        this->running_on = size_t(-1);
        this->tid = this->parent->alloc_tid();
    }

    thread::thread(process *parent, uintptr_t pc, uintptr_t arg, ssize_t run_on) :
        self(this), error(no_error), parent(parent),
        user(false), in_queue(false), status(status::dequeued), run_on(run_on)
    {
        this->running_on = size_t(-1);
        this->tid = this->parent->alloc_tid();

        thread_finalise(this, pc, arg);
        parent->threads.push_back(this);
    }

    thread::thread(process *parent, uintptr_t pc, std::span<std::string_view> argv, std::span<std::string_view> envp, elf::exec::auxval auxv) :
        self(this), error(no_error), parent(parent),
        user(true), in_queue(false), status(status::dequeued), run_on(-1)
    {
        this->running_on = size_t(-1);
        this->tid = this->parent->alloc_tid();

        auto [vstack, vustack] = map_user_stack(this, parent);
        this->stack = elf::exec::prepare_stack(vstack, vustack, argv, envp, auxv);

        thread_finalise(this, pc, 0);
        parent->threads.push_back(this);
    }

    thread::~thread()
    {
        // auto it = std::find(this->parent->threads.begin(), this->parent->threads.end(), this);
        // if (it != this->parent->threads.end())
        //     this->parent->threads.erase(it);

        erase_from(this->parent->threads, this);

        if (this->parent->threads.empty())
        {
            if (this->parent->exited == false)
                pexit(this->parent, 0);

            delete this->parent->pagemap;
        }

        for (const auto &stack : this->stacks)
            pmm::free(stack.first, stack.second / pmm::page_size);

        thread_delete(this);
    }

    static void enqueue_notready(thread *thread)
    {
        std::unique_lock guard(lock);
        thread->in_queue = true;
        if (thread->status == status::running)
            thread->status = status::ready;
        expired->push_front(thread);
    }

    static void scheduler(cpu::registers_t *regs)
    {
        auto new_thread = next_thread(this_cpu()->id);
        while (new_thread != nullptr && new_thread->status != status::ready)
        {
            if (new_thread->status == status::killed)
            {
                delete new_thread;
                continue;
            }

            if (new_thread->events.empty() == false && new_thread->timeout > 0)
            {
                if (size_t(new_thread->timeout) <= time::time_ms())
                {
                    new_thread->timeout = 0;
                    event::trigger(new_thread->events[0]);
                    continue;
                }
            }

            enqueue_notready(new_thread);
            new_thread = next_thread(this_cpu()->id);
        }

        if (new_thread == nullptr)
            new_thread = this_cpu()->idle;
        else
            new_thread->status = status::running;

        auto current_thread = this_thread();
        if (current_thread != nullptr && current_thread->status != status::killed)
        {
            if (current_thread != this_cpu()->idle)
                enqueue_notready(current_thread);

            // if (current_thread->status == status::running && current_thread != this_cpu()->idle)
            //     enqueue(current_thread);

            save_thread(current_thread, regs);
        }

        reschedule(fixed_timeslice);
        load_thread(new_thread, regs);

        if (current_thread != nullptr && current_thread->status == status::killed && current_thread != this_cpu()->idle)
            delete current_thread;
    }

    std::pair<pid_t, tid_t> pid()
    {
        auto thread = this_thread();
        return { thread->parent->pid, thread->tid };
    }

    std::tuple<std::string_view, pid_t, tid_t> pid_name()
    {
        auto thread = this_thread();
        return { thread->parent->name, thread->parent->pid, thread->tid };
    }

    // Pid 0 is for kernel process
    // Pid -1 and down are for idle processes
    static std::atomic<pid_t> idle_pids(-1);

    bool initialised = false;
    [[noreturn]] void init(bool start)
    {
        arch_init(scheduler);

        auto proc = new process();
        proc->pid = idle_pids--;
        proc->name = "Idle Process for CPU: ";
        proc->name += std::to_string(this_cpu()->id);
        proc->pagemap = vmm::kernel_pagemap;

        auto idle_thread = new thread(proc, arch::halt, true);
        idle_thread->status = status::ready;
        this_cpu()->idle = idle_thread;

        if (start == true)
            wake_up(0, initialised = true);

        arch::halt();
    }
} // namespace proc