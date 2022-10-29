// Copyright (C) 2022  ilobilo

#pragma once

#include <drivers/vfs.hpp>
#include <cpu/cpu.hpp>
#include <mm/vmm.hpp>

namespace proc
{
    static constexpr uintptr_t def_usr_stack_top = 0x70000000000;
    static constexpr size_t fixed_timeslice = 6;

    enum class status
    {
        dequeued = 0,
        enqueued = 1,

        ready = enqueued,
        running = 2,

        blocked = 3,
        killed = 4
    };

    struct thread;
    struct process
    {
        pid_t pid;
        std::string name;

        vmm::pagemap *pagemap;

        std::atomic<tid_t> next_tid;

        vfs::node_t *root;
        vfs::node_t *cwd;

        mode_t umask;

        gid_t gid;
        gid_t sgid;
        gid_t egid;

        uid_t uid;
        uid_t suid;
        uid_t euid;

        process *parent;
        std::vector<thread*> threads;
        std::vector<process*> children;

        uintptr_t usr_stack_top;

        process() : name(""), pagemap(nullptr), next_tid(1), parent(nullptr), usr_stack_top(def_usr_stack_top) { }

        process(std::string_view name);
        process(std::string_view name, process *old_proc);

        process(process *old_proc) : process(old_proc->name, old_proc) { }

        tid_t alloc_tid();
    };

    struct thread
    {
        // DO NOT MOVE: START
        size_t running_on;
        thread *self;
        uintptr_t stack;

        #if defined(__x86_64__)
        uintptr_t kstack;
        #endif
        // DO NOT MOVE: END

        tid_t tid;
        errno_t error;
        process *parent;

        bool user;
        bool in_queue;
        status status;

        cpu::registers_t regs;
        std::vector<uintptr_t> stacks;

        #if defined(__x86_64__)
        uintptr_t gs_base;
        uintptr_t fs_base;

        size_t fpu_storage_pages;
        uint8_t *fpu_storage;
        #elif defined(__aarch64__)
        uintptr_t el0_base;
        #endif

        thread(process *parent, uintptr_t pc, uintptr_t arg, bool user);
        thread(process *parent, auto pc, auto arg, bool user) : thread(parent, uintptr_t(pc), uintptr_t(arg), user) { }

        ~thread();
    };

    extern std::unordered_map<pid_t, process*> processes;

    inline pid_t alloc_pid()
    {
        for (pid_t i = 0; i < std::numeric_limits<pid_t>::max(); i++)
            if (processes.contains(i) == false)
                return i;

        return -1;
    }

    inline bool free_pid(pid_t pid)
    {
        return processes.erase(pid);
    }

    thread *next_thread();

    void yield();

    void enqueue(thread *thread);

    void dequeue(thread *thread);
    void dequeue();

    void unblock(thread *thread);

    void block(thread *thread);
    void block();

    void exit(thread *thread);
    void exit();

    [[noreturn]] void init(bool start = false);
} // namespace proc

extern "C" proc::thread *this_thread();