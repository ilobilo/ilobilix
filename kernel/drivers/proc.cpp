// Copyright (C) 2022  ilobilo

#include <drivers/proc.hpp>
#include <drivers/smp.hpp>
#include <mm/pmm.hpp>
#include <deque>

// #include <lib/log.hpp>

namespace proc
{
    std::unordered_map<pid_t, process*> processes;
    static lock_t pid_lock;
    static lock_t lock;

    void thread_finalise(thread *thread, uintptr_t pc, uintptr_t arg, std::span<std::string_view> argv, std::span<std::string_view> envp, elf::exec::auxval auxv);
    void thread_delete(thread *thread);

    void save_thread(thread *thread, cpu::registers_t *regs);
    void load_thread(thread *thread, cpu::registers_t *regs);
    void wake_up(size_t id, bool everyone = false);
    void reschedule(uint64_t ms = 0);

    void arch_init(void (*func)(cpu::registers_t *regs));

    // O(1) scheduling algorithm (used in linux kernel versions 2.6.0-2.6.22)
    static std::deque<thread*> queues[2];
    static auto active = &queues[0];
    static auto expired = &queues[1];

    thread *next_thread()
    {
        lockit(lock);

        if (active->size() == 0)
            std::swap(active, expired);

        if (active->size() == 0)
            return nullptr;

        auto ret = active->pop_back_element();
        ret->in_queue = false;

        return ret;
    }

    void yield()
    {
        reschedule();
    }

    void enqueue(thread *thread)
    {
        lockit(lock);
        if (thread->in_queue == true)
            return;

        thread->in_queue = true;
        thread->status = status::ready;
        expired->push_front(thread);
    }

    void dequeue(thread *thread)
    {
        lockit(lock);
        thread->status = status::dequeued;
    }

    void dequeue()
    {
        dequeue(this_thread());
    }

    void unblock(thread *thread)
    {
        lockit(lock);
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
        // log::infoln("Scheduler: Blocking thread {}:{}", thread->parent->pid, thread->tid);

        auto status = thread->status;
        auto running_on = thread->running_on;

        thread->status = status::blocked;

        lock.unlock();
        if (status == status::running)
            wake_up(running_on);
    }

    void block()
    {
        block(this_thread());
    }

    void exit(thread *thread)
    {
        lock.lock();
        // log::infoln("Scheduler: Exiting thread {}:{}", thread->parent->pid, thread->tid);

        auto status = thread->status;
        auto running_on = thread->running_on;

        thread->status = status::killed;

        lock.unlock();
        if (status == status::running)
            wake_up(running_on);
    }

    void exit()
    {
        exit(this_thread());
    }

    void pexit(process *proc)
    {
        lock.lock();
        // log::infoln("Scheduler: Exiting process {}", proc->pid);

        auto status = status::killed;
        size_t running_on = 0;

        for (const auto &thread : proc->threads)
        {
            if (thread->status == status::running)
            {
                status = status::running;
                running_on = thread->running_on;
            }
            thread->status = status::killed;
        }

        lock.unlock();
        if (status == status::running)
            wake_up(running_on);
    }

    void pexit()
    {
        pexit(this_thread()->parent);
    }

    process::process(std::string_view name) : name(name), pagemap(nullptr), next_tid(1), parent(nullptr), usr_stack_top(def_usr_stack_top)
    {
        this->root = vfs::get_root();
        this->cwd = vfs::get_root();
        this->umask = s_iwgrp | s_iwoth;

        this->gid = 0;
        this->sgid = 0;
        this->egid = 0;

        this->uid = 0;
        this->suid = 0;
        this->euid = 0;

        lockit(pid_lock);
        this->pid = alloc_pid();
        processes[this->pid] = this;
    }

    process::process(std::string_view name, process *old_proc) : name(name), next_tid(1), parent(old_proc), usr_stack_top(old_proc->usr_stack_top)
    {
        this->root = old_proc->root;
        this->cwd = old_proc->cwd;
        this->umask = old_proc->umask;
        // this->pagemap = old_proc->pagemap.fork();

        this->gid = old_proc->gid;
        this->sgid = old_proc->sgid;
        this->egid = old_proc->egid;

        this->uid = old_proc->uid;
        this->suid = old_proc->suid;
        this->euid = old_proc->euid;

        lockit(pid_lock);
        this->pid = alloc_pid();
        processes[this->pid] = this;
    }

    tid_t process::alloc_tid()
    {
        return this->next_tid++;
    }

    process::~process()
    {
        vmm::kernel_pagemap->load();

        auto pid1 = processes[1];
        for (const auto &proc : this->children)
        {
            proc->parent = pid1;
            pid1->children.push_back(proc);
            this->children.erase(&proc);
        }

        for (const auto &thread : this->threads)
            exit(thread);

        processes.erase(this->pid);
    }

    static std::pair<uintptr_t, uintptr_t> map_user_stack(thread *thread, process *parent)
    {
        uintptr_t pstack = pmm::alloc<uintptr_t>(default_stack_size / pmm::page_size);
        uintptr_t vustack = parent->usr_stack_top - default_stack_size;

        parent->pagemap->map_range(vustack, pstack, default_stack_size, vmm::rwu);
        parent->usr_stack_top = vustack - pmm::page_size;

        thread->stacks.push_back(pstack);
        return { tohh(pstack) + default_stack_size, vustack + default_stack_size };
    }

    thread::thread(process *parent, uintptr_t pc, uintptr_t arg) : self(this), error(no_error), parent(parent), user(false), in_queue(false), status(status::dequeued)
    {
        this->running_on = size_t(-1);
        this->tid = this->parent->alloc_tid();

        if (this->user == true)
            this->stack = map_user_stack(this, parent).second;

        thread_finalise(this, pc, arg, std::span<std::string_view>(), std::span<std::string_view>(), elf::exec::auxval { });
        parent->threads.push_back(this);
    }

    thread::thread(process *parent, uintptr_t pc, uintptr_t arg, std::span<std::string_view> argv, std::span<std::string_view> envp, elf::exec::auxval auxv) : self(this), error(no_error), parent(parent), user(true), in_queue(false), status(status::dequeued)
    {
        this->running_on = size_t(-1);
        this->tid = this->parent->alloc_tid();

        if (this->user == true)
        {
            auto [vstack, vustack] = map_user_stack(this, parent);
            this->stack = elf::exec::prepare_stack(vstack, vustack, argv, envp, auxv);
        }

        thread_finalise(this, pc, arg, argv, envp, auxv);
        parent->threads.push_back(this);
    }

    thread::~thread()
    {
        auto it = std::find(this->parent->threads.begin(), this->parent->threads.end(), this);
        if (it != this->parent->threads.end())
            this->parent->threads.erase(it);

        if (this->parent->threads.empty())
            delete this->parent;

        thread_delete(this);
    }

    static void scheduler(cpu::registers_t *regs)
    {
        auto new_thread = next_thread();
        while (new_thread != nullptr && new_thread->status != status::ready)
        {
            if (new_thread->status == status::killed)
                delete new_thread;

            new_thread = next_thread();
        }

        if (new_thread == nullptr)
            new_thread = this_cpu()->idle;
        else
            new_thread->status = status::running;

        auto current_thread = this_thread();
        if (current_thread != nullptr && current_thread->status != status::killed)
        {
            if (current_thread->status == status::running && current_thread != this_cpu()->idle)
                enqueue(current_thread);

            save_thread(current_thread, regs);
        }

        // log::infoln("Scheduler: Running '{}' {}:{} on CPU: {}", new_thread->parent->name.c_str(), new_thread->parent->pid, new_thread->tid, this_cpu()->id);

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

    // Pid 0 is for kernel process
    // Pid -1 and down are for idle processes
    static std::atomic<pid_t> idle_pids(-1);

    [[noreturn]] void init(bool start)
    {
        arch_init(scheduler);

        auto proc = new process();
        proc->pid = idle_pids--;
        proc->name = "Idle Process for CPU: ";
        proc->name += std::to_string(this_cpu()->id);
        proc->pagemap = vmm::kernel_pagemap;

        auto idle_thread = new thread(proc, (void (*)())([]() { arch::halt(); }), 0);
        idle_thread->status = status::ready;
        this_cpu()->idle = idle_thread;

        if (start == true)
            wake_up(0, true);

        arch::halt();
    }
} // namespace proc