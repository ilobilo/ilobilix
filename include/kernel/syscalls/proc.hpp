namespace proc
{
    uintptr_t sys_exit(int)
    {
        proc::pexit();
        return 0;
    }

    uintptr_t sys_getpid()
    {
        return this_thread()->parent->pid;
    }

    uintptr_t sys_getppid()
    {
        auto proc = this_thread()->parent;
        return proc->parent ? proc->parent->pid : 1;
    }

    uintptr_t sys_umask(mode_t mask)
    {
        auto proc = this_thread()->parent;
        auto old_mask = proc->umask;
        proc->umask = mask;
        return old_mask;
    }

    uintptr_t sys_uname(utsname *buf)
    {
        strncpy(buf->sysname, "Ilobilix", sizeof(buf->sysname));
        strncpy(buf->nodename, "ilobilix", sizeof(buf->nodename));
        strncpy(buf->release, "0.0.1", sizeof(buf->release));
        strncpy(buf->version, __DATE__ " " __TIME__, sizeof(buf->version));
        strncpy(buf->machine, "", sizeof(buf->machine));
        strncpy(buf->domainname, "", sizeof(buf->domainname));
        return 0;
    }
} // namespace proc