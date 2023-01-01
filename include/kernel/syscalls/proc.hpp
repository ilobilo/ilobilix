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
        return this_thread()->parent->parent->pid;
    }
} // namespace proc