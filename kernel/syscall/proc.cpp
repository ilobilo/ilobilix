// Copyright (C) 2022-2023  ilobilo

#include <syscall/proc.hpp>
#include <drivers/proc.hpp>
#include <mm/pmm.hpp>

namespace proc
{
    int sys_exit(int code)
    {
        proc::pexit(code);
        return 0;
    }

    pid_t sys_getpid()
    {
        return this_thread()->parent->pid;
    }

    pid_t sys_getppid()
    {
        auto proc = this_thread()->parent;
        return proc->parent ? proc->parent->pid : 0;
    }

    mode_t sys_umask(mode_t mask)
    {
        auto proc = this_thread()->parent;
        auto old_mask = proc->umask.get();
        // proc->umask.get() = mask;
        proc->umask = mask;
        return old_mask;
    }

    int sys_uname(utsname *buf)
    {
        strncpy(buf->sysname, "Ilobilix", sizeof(buf->sysname));
        strncpy(buf->nodename, "ilobilix", sizeof(buf->nodename));
        strncpy(buf->release, "0.0.1", sizeof(buf->release));
        strncpy(buf->version, __DATE__ " " __TIME__, sizeof(buf->version));
        strncpy(buf->machine, "", sizeof(buf->machine));
        strncpy(buf->domainname, "", sizeof(buf->domainname));
        return 0;
    }

    pid_t clone(kernel_clone_args args)
    {
        if (((args.flags & clone_sighand) == clone_sighand && (args.flags & clone_vm) != clone_vm) ||
            ((args.flags & clone_thread) == clone_thread && (args.flags & clone_sighand) != clone_sighand) ||
            ((args.flags & clone_fs) == clone_fs && (args.flags & clone_newnet) == clone_newnet) ||
            ((args.flags & clone_newipc) == clone_newipc && (args.flags & clone_sysvsem) == clone_sysvsem) ||
            ((args.flags & clone_newpid) == clone_newpid && (args.flags & clone_thread) == clone_thread) ||
            ((args.flags & clone_vm) == clone_vm && args.stack == 0))
            return_err(-1, EINVAL);

        auto old_thread = this_thread();
        auto old_proc = old_thread->parent;

        auto new_proc = new process(old_proc->name);
        new_proc->parent = old_proc;

        auto new_thread = new thread(new_proc);
        new_thread->user = true;

        if ((args.flags & clone_fs) == clone_fs)
        {
            new_proc->root = old_proc->root;
            new_proc->cwd = old_proc->cwd;
            new_proc->umask = old_proc->umask;
        }
        else
        {
            new_proc->root = old_proc->root.get();
            new_proc->cwd = old_proc->cwd.get();
            new_proc->umask = old_proc->umask.get();
        }

        if ((args.flags & clone_files) == clone_files)
        {
            std::unique_lock guard(old_proc->fd_table->lock);
            new_proc->fd_table = old_proc->fd_table;
        }
        else
        {
            new_proc->fd_table = std::make_shared<vfs::fd_table>();
            for (const auto [num, fd] : old_proc->fd_table->fds)
                old_proc->dupfd(num, new_proc, num, 0, true, false);
        }
        new_proc->session = old_proc->session;

        if ((args.flags & clone_parent) == clone_parent)
            new_proc->parent = old_proc->parent;
        else
            new_proc->parent = old_proc;

        // TODO: sigmask
        // TODO: sigactions

        new_thread->regs = old_thread->saved_regs;

        if ((args.flags & clone_vm) == clone_vm)
        {
            new_proc->pagemap = old_proc->pagemap;
            new_thread->stack = args.stack;
#if defined(__x86_64__)
            new_thread->regs.rsp = new_thread->stack;
#endif
        }
        else
        {
            new_proc->pagemap = new vmm::pagemap(old_proc->pagemap);
            new_thread->stack = old_thread->stack;
        }

        if ((args.flags & clone_child_settid) == clone_child_settid && args.child_tid != nullptr)
            *args.child_tid = new_thread->tid;

        if ((args.flags & clone_parent_settid) == clone_parent_settid && args.parent_tid != nullptr)
            *args.parent_tid = old_thread->tid;

        new_proc->gid = old_proc->gid;
        new_proc->sgid = old_proc->sgid;
        new_proc->egid = old_proc->egid;

        new_proc->uid = old_proc->uid;
        new_proc->suid = old_proc->suid;
        new_proc->euid = old_proc->euid;

        new_proc->usr_stack_top = old_proc->usr_stack_top;

#if defined(__x86_64__)
        new_thread->gs_base = old_thread->gs_base;
        if ((args.flags & clone_settls) == clone_settls)
            new_thread->fs_base = args.tls;
        else
            new_thread->fs_base = old_thread->fs_base;

        uintptr_t pkstack = pmm::alloc<uintptr_t>(kernel_stack_size / pmm::page_size);
        new_thread->kstack = tohh(pkstack) + kernel_stack_size;
        new_thread->stacks.push_back(std::make_pair(pkstack, kernel_stack_size));

        uintptr_t ppfstack = pmm::alloc<uintptr_t>(kernel_stack_size / pmm::page_size);
        new_thread->pfstack = tohh(ppfstack) + kernel_stack_size;
        new_thread->stacks.push_back(std::make_pair(ppfstack, kernel_stack_size));

        new_thread->fpu_storage_pages = old_thread->fpu_storage_pages;
        new_thread->fpu_storage = tohh(pmm::alloc<uint8_t*>(new_thread->fpu_storage_pages));
        memcpy(new_thread->fpu_storage, old_thread->fpu_storage, new_thread->fpu_storage_pages * pmm::page_size);

        new_thread->regs.rax = 0;
        new_thread->regs.rdx = 0;
#elif defined(__aarch64__)
#endif

        new_proc->threads.push_back(new_thread);
        old_proc->children.push_back(new_proc);

        enqueue(new_thread);
        return new_proc->pid;
    }

    pid_t sys_clone(uint64_t clone_flags, uintptr_t newsp, int *parent_tidptr, int *child_tidptr, uintptr_t tls)
    {
        return clone({
            .flags = uint32_t(clone_flags) & ~csignal,
            .pidfd = parent_tidptr,
            .child_tid = child_tidptr,
            .parent_tid = parent_tidptr,
            .exit_signal = uint32_t(clone_flags) & csignal,
            .stack = newsp,
            .tls = tls,
        });
    }

    pid_t sys_fork()
    {
        return sys_clone(0 /* SIGCHLD */, 0, 0, 0, 0);
/*
        auto old_thread = this_thread();
        auto old_proc = old_thread->parent;

        auto new_proc = new process(old_proc->name);

        new_proc->pagemap = new vmm::pagemap(old_proc->pagemap);
        new_proc->parent = old_proc;

        new_proc->root = old_proc->root.get();
        new_proc->cwd = old_proc->cwd.get();
        new_proc->umask = old_proc->umask.get();

        new_proc->fd_table = std::make_shared<vfs::fd_table>();
        for (const auto [num, fd] : old_proc->fd_table->fds)
            old_proc->dupfd(num, new_proc, num, 0, true, false);

        new_proc->session = old_proc->session;

        new_proc->gid = old_proc->gid;
        new_proc->sgid = old_proc->sgid;
        new_proc->egid = old_proc->egid;

        new_proc->uid = old_proc->uid;
        new_proc->suid = old_proc->suid;
        new_proc->euid = old_proc->euid;

        new_proc->usr_stack_top = old_proc->usr_stack_top;

        auto new_thread = new thread(new_proc);

        new_thread->regs = old_thread->saved_regs;
        new_thread->user = true;

        new_thread->stack = old_thread->stack;

#if defined(__x86_64__)
        new_thread->gs_base = old_thread->gs_base;
        new_thread->fs_base = old_thread->fs_base;

        uintptr_t pkstack = pmm::alloc<uintptr_t>(kernel_stack_size / pmm::page_size);
        new_thread->kstack = tohh(pkstack) + kernel_stack_size;
        new_thread->stacks.push_back(std::make_pair(pkstack, kernel_stack_size));

        uintptr_t ppfstack = pmm::alloc<uintptr_t>(kernel_stack_size / pmm::page_size);
        new_thread->pfstack = tohh(ppfstack) + kernel_stack_size;
        new_thread->stacks.push_back(std::make_pair(ppfstack, kernel_stack_size));

        new_thread->fpu_storage_pages = old_thread->fpu_storage_pages;
        new_thread->fpu_storage = tohh(pmm::alloc<uint8_t*>(new_thread->fpu_storage_pages));
        memcpy(new_thread->fpu_storage, old_thread->fpu_storage, new_thread->fpu_storage_pages * pmm::page_size);

        new_thread->regs.rax = 0;
        new_thread->regs.rdx = 0;
#endif

        new_proc->threads.push_back(new_thread);
        old_proc->children.push_back(new_proc);

        enqueue(new_thread);
        return new_proc->pid;
*/
    }

    int sys_execve(const char *pathname, char *const argv[], char *const envp[])
    {
        auto delret = [](auto &var, auto ret) { delete var; return ret; };

        auto old_thread = this_thread();
        auto old_proc = old_thread->parent;

        auto node = std::get<1>(vfs::path2node(old_proc->cwd, pathname));
        if (node == nullptr)
            return -1;

        node = node->reduce(true);
        if (node == nullptr)
            return -1;

        if (node->res->stat.has_access(old_proc->euid, old_proc->egid, stat_t::exec) == false)
            return_err(-1, EACCES);

        auto is_suid = node->res->stat.st_mode & s_isuid ? true : false;
        auto is_sgid = node->res->stat.st_mode & s_isgid ? true : false;

        auto old_pagemap = old_proc->pagemap;
        auto new_pagemap = new vmm::pagemap();

        auto ret = elf::exec::load(node->res, new_pagemap, 0);
        if (ret.has_value() == false)
            return delret(new_pagemap, -1);

        auto [auxv, ld_path] = ret.value();
        uintptr_t entry = 0;

        if (ld_path.empty() == false)
        {
            auto ld_node = std::get<1>(vfs::path2node(nullptr, ld_path))->reduce(true);
            if (ld_node == nullptr)
                return delret(new_pagemap, -1);

            ld_node = ld_node->reduce(true);
            if (ld_node == nullptr)
                return delret(new_pagemap, -1);

            if (auto ld_ret = elf::exec::load(ld_node->res, new_pagemap, 0x40000000); ld_ret.has_value())
                entry = ld_ret.value().first.at_entry;
            else
                return delret(new_pagemap, -1);
        }
        else entry = auxv.at_entry;

        std::vector<std::string_view> argvs;
        while (true)
        {
            if (argv[argvs.size()] == nullptr)
                break;
            argvs.emplace_back(strdup(argv[argvs.size()]));
        }

        std::vector<std::string_view> envps;
        while (true)
        {
            if (envp[envps.size()] == nullptr)
                break;
            envps.emplace_back(strdup(envp[envps.size()]));
        }

        remove_from(old_proc->parent->children, old_proc);

        for (const auto &thread : old_proc->threads)
            if (thread != old_thread)
                thread->status = status::killed;

        old_proc->name = node->to_path();
        old_proc->pagemap = new_pagemap;
        old_proc->usr_stack_top = def_usr_stack_top;

        if (is_suid == true)
            old_proc->euid = node->res->stat.st_uid;
        if (is_sgid == true)
            old_proc->egid = node->res->stat.st_gid;

        enqueue(new thread(old_proc, entry, std::span<std::string_view>(argvs.begin(), argvs.size()), std::span<std::string_view>(envps.begin(), envps.size()), auxv));

        vmm::kernel_pagemap->load();
        delete old_pagemap;

        exit();

        return 0;
    }

    pid_t sys_wait4(pid_t pid, int *wstatus, int options, rusage *rusage)
    {
        // TODO: rusage

        auto thread = this_thread();
        auto proc = thread->parent;

        // TODO: zombies
        // for (auto it = proc->zombies.begin(); it != proc->zombies.end(); it++)
        // {
        //     auto zombie = *it;
        //     if (pid < -1)
        //     {
        //         // TODO: if(zombie pgid != zombie pid) continue
        //     }
        //     else if (pid == 0)
        //     {
        //         // TODO: if (zombie pgid != proc pgid) continue
        //     }
        //     else if (pid > 0)
        //     {
        //         if (zombie->pid != pid)
        //             continue;
        //     }

        //     if (wstatus != nullptr)
        //         *wstatus = w_exitcode(zombie->status, 0);

        //     it = proc->zombies.erase(it);
        //     // TODO: delete zombie?
        //     return zombie->pid;
        // }

        if ((options & wnohang) == wnohang && proc->zombies.empty())
            return 0;

        std::vector<process*> procs;
        if (pid > 0)
        {
            auto it = processes.find(pid);
            if (it == processes.end() || it->second->parent != proc)
                return_err(-1, ECHILD);
            procs.push_back(it->second);
        }
        else
        {
            if (proc->children.empty())
                return_err(-1, ECHILD);

            if (pid < -1)
            {
                for (const auto &child : proc->children)
                    // TODO: if (child pgid == abs(pid))
                    procs.push_back(child);
            }
            else if (pid == -1)
            {
                for (const auto &child : proc->children)
                    procs.push_back(child);
            }
            else if (pid == 0)
            {
                for (const auto &child : proc->children)
                    // TODO: if (child pgid == proc pgid)
                    procs.push_back(child);
            }
        }

        std::vector<event_t*> events;
        for (const auto &child : procs)
            events.push_back(&child->event);

        while (true)
        {
            auto ret = event::await(std::span(events.begin(), events.end()));
            if (ret.has_value() == false)
                return_err(-1, EINTR);

            auto which = procs[ret.value()];

            if (!(options & wuntraced) && wifstopped(which->status))
                continue;
            if (!(options & wcontinued) && wifcontinued(which->status))
                continue;

            if (wstatus != nullptr)
                *wstatus = w_exitcode(which->status, 0);
            return which->pid;
        }
    }
} // namespace proc