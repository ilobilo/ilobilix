// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <lib/types.hpp>

namespace proc
{
    int sys_exit(int code);

    pid_t sys_getpid();
    pid_t sys_getppid();

    mode_t sys_umask(mode_t mask);
    int sys_uname(utsname *buf);

    pid_t sys_clone(uint64_t clone_flags, uintptr_t newsp, int *parent_tidptr, int *child_tidptr, uintptr_t tls);
    pid_t sys_fork();

    int sys_execve(const char *pathname, char *const argv[], char *const envp[]);

    pid_t sys_wait4(pid_t pid, int *wstatus, int options, rusage *rusage);
} // namespace proc