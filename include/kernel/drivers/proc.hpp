// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <drivers/fs/dev/tty/tty.hpp>
#include <drivers/vfs.hpp>
#include <drivers/elf.hpp>
#include <drivers/fd.hpp>
#include <lib/event.hpp>
#include <lib/misc.hpp>
#include <cpu/cpu.hpp>
#include <mm/vmm.hpp>

namespace proc
{
    inline constexpr uintptr_t def_usr_stack_top = 0x70000000000;
    inline constexpr size_t fixed_timeslice = 6;

    enum class status
    {
        dequeued = 0,
        enqueued = 1,

        ready = enqueued,
        running = 2,

        blocked = 3,
        killed = 4
    };

    struct session
    {
        tty::tty_t *tty;
    };

    struct thread;
    struct process
    {
        pid_t pid;
        std::string name;

        vmm::pagemap *pagemap;

        std::atomic<tid_t> next_tid;

        chain_wrapper<vfs::node_t*> root;
        chain_wrapper<vfs::node_t*> cwd;
        chain_wrapper<mode_t> umask;

        std::shared_ptr<vfs::fd_table> fd_table;
        std::shared_ptr<session> session;

        gid_t gid;
        gid_t sgid;
        gid_t egid;

        uid_t uid;
        uid_t suid;
        uid_t euid;

        process *parent;
        std::vector<thread*> threads;
        std::vector<process*> children;
        std::vector<process*> zombies;

        int status;
        bool exited;
        event_t event;

        uintptr_t usr_stack_top;

        process() : name(""), pagemap(nullptr), next_tid(1), root(nullptr), cwd(nullptr), umask(0), fd_table(nullptr), session(nullptr), parent(nullptr), status(0), exited(false), usr_stack_top(def_usr_stack_top) { }
        process(std::string_view name);

        tid_t alloc_tid();

        int dupfd(int old_num, process *new_proc, int new_num, int flags, bool specific, bool cloexec);
        inline int dupfd(int old_num, int new_num, int flags, bool specific, bool cloexec)
        {
            return this->dupfd(old_num, this, new_num, flags, specific, cloexec);
        }

        int open(int dirfd, std::string_view pathname, int flags, mode_t mode, int spec_fd = -1);
    };

    struct thread
    {
        // DO NOT MOVE: START
        size_t running_on;
        thread *self;
        uintptr_t stack;

#if defined(__x86_64__)
        uintptr_t kstack;
        uintptr_t pfstack;
#endif
        // DO NOT MOVE: END

        tid_t tid;
        errno_t error;
        process *parent;

        bool user;
        bool in_queue;
        status status;

        cpu::registers_t regs;
        cpu::registers_t saved_regs;

        std::vector<std::pair<uintptr_t, size_t>> stacks;

#if defined(__x86_64__)
        uintptr_t gs_base;
        uintptr_t fs_base;

        size_t fpu_storage_pages;
        uint8_t *fpu_storage;
#elif defined(__aarch64__)
        uintptr_t el0_base;
#endif

        std::deque<event_t*> events;
        ssize_t timeout = -1;
        size_t event = 0;

        thread(process *parent);

        thread(process *parent, uintptr_t pc, uintptr_t arg = 0);
        thread(process *parent, uintptr_t pc, std::span<std::string_view> argv, std::span<std::string_view> envp, elf::exec::auxval auxv);

        thread(process *parent, auto pc, auto arg = 0) : thread(parent, uintptr_t(pc), uintptr_t(arg)) { }
        thread(process *parent, auto pc, std::span<std::string_view> argv, std::span<std::string_view> envp, elf::exec::auxval auxv) : thread(parent, uintptr_t(pc), argv, envp, auxv) { }

        ~thread();
    };

    extern std::unordered_map<pid_t, process*> processes;

    inline std::mutex pid_lock;
    inline pid_t alloc_pid(process *proc)
    {
        std::unique_lock guard(pid_lock);
        for (pid_t i = 0; i < std::numeric_limits<pid_t>::max(); i++)
        {
            if (processes.contains(i) == false)
            {
                processes[i] = proc;
                return i;
            }
        }
        return -1;
    }

    inline bool free_pid(pid_t pid)
    {
        std::unique_lock guard(pid_lock);
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
    [[noreturn]] void exit();

    void pexit(process *proc, int code);
    [[noreturn]] void pexit(int code);

    std::pair<pid_t, tid_t> pid();
    std::tuple<std::string_view, pid_t, tid_t> pid_name();

    extern bool initialised;
    [[noreturn]] void init(bool start = false);
} // namespace proc

extern "C" proc::thread *this_thread();