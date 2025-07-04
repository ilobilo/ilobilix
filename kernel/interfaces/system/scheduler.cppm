// Copyright (C) 2024-2025  ilobilo

export module system.scheduler;

import system.scheduler.base;
import system.memory.virt;
import system.cpu.self;
import system.cpu;
import system.vfs;
import lib;
import cppstd;

extern "C++" struct processor;
export namespace sched
{
    constexpr std::size_t timeslice = 6;

    enum class status
    {
        not_ready,
        ready,
        running,
        blocked,
        sleeping,
        killed
    };

    enum wake_reason
    {
        success = 0,
        interrupted = 1
    };

    struct process;
    struct thread : thread_base
    {
        std::uintptr_t ustack_top;
        std::uintptr_t kstack_top;

        std::size_t tid;
        std::size_t pid;

        status status;
        bool is_user;

        cpu::registers regs;

        std::uint64_t vruntime;
        std::uint64_t schedule_time;

        lib::spinlock sleep_lock;
        bool sleep_ints;
        std::size_t wake_reason;
        std::optional<std::size_t> sleep_for;
        std::optional<std::size_t> sleep_until;

#if defined(__x86_64__)
        std::uintptr_t pfstack_top;

        std::uintptr_t gs_base;
        std::uintptr_t fs_base;

        std::byte *fpu;
#elif defined(__aarch64__)
#endif

        static std::uintptr_t allocate_ustack(std::shared_ptr<process> &proc);
        static std::uintptr_t allocate_kstack(std::shared_ptr<process> &proc);

        static std::shared_ptr<thread> create(std::shared_ptr<process> &parent, std::uintptr_t ip, std::uintptr_t arg = 0);

        void prepare_sleep(std::size_t ms = 0);
        bool wake_up(std::size_t reason);

        thread() = default;
        ~thread();
    };

    struct process
    {
        static constexpr std::uintptr_t initial_stck_top = 0x70000000000;
        static constexpr std::uintptr_t initial_mmap_anon = 0x80000000000;

        std::size_t pid;
        // std::size_t pgid;
        // std::size_t sid;

        gid_t gid = 0, sgid = 0, egid = 0;
        uid_t uid = 0, suid = 0, euid = 0;

        std::shared_ptr<vmm::vmspace> vmspace;

        std::shared_ptr<vfs::dentry> root;
        std::shared_ptr<vfs::dentry> cwd;
        mode_t umask = static_cast<mode_t>(fmode::s_iwgrp | fmode::s_iwoth);
        // TODO: fd table

        lib::spinlock lock;

        std::weak_ptr<process> parent;
        lib::map::flat_hash<std::size_t, std::shared_ptr<process>> children;
        lib::map::flat_hash<std::size_t, std::shared_ptr<thread>> threads;

        std::atomic<std::size_t> next_tid = 1;
        std::uintptr_t next_stack_top = initial_stck_top;
        std::uintptr_t mmap_anon_base = initial_mmap_anon;

        static std::shared_ptr<process> create(std::shared_ptr<process> parent, std::shared_ptr<vmm::pagemap> pagemap);

        void prepare_sleep();

        process() = default;
        ~process();
    };

    struct percpu
    {
        struct compare
        {
            constexpr bool operator()(const std::shared_ptr<thread> &lhs, const std::shared_ptr<thread> &rhs) const
            {
                return lhs->vruntime < rhs->vruntime;
            }
        };

        lib::spinlock_preempt lock;

        lib::btree::multiset<std::shared_ptr<thread>, compare> queue;
        std::shared_ptr<thread> running_thread;

        std::shared_ptr<process> idle_proc;
        std::shared_ptr<thread> idle_thread;
    };
    cpu_local<percpu> percpu;

    std::shared_ptr<process> &proc_for(std::size_t pid);

    bool initialised = false;

    std::shared_ptr<thread> this_thread();
    std::size_t yield();

    std::size_t allocate_cpu();
    void enqueue(std::shared_ptr<thread> &thread, std::size_t cpu_idx);

    void enable();
    void disable();
    bool is_enabled();

    [[noreturn]] void start();

    initgraph::stage *available_stage();
} // export namespace sched