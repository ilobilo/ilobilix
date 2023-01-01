namespace vmm
{
    void *sys_mmap(void *addr, size_t length, int prot, int flags, int fdnum, off_t offset)
    {
        auto proc = this_thread()->parent;
        auto pagemap = proc->pagemap;
        vfs::resource *res = nullptr;

        if (fdnum != -1)
        {
            auto fd = proc->num2fd(fdnum);
            if (fd == nullptr)
                return MAP_FAILED;
            res = fd->handle->res;
        }
        else if (offset != 0)
            return_err(MAP_FAILED, EINVAL);

        return pagemap->mmap(uintptr_t(addr), length, prot, flags, res, offset);
    }

    int sys_munmap(void *addr, size_t length)
    {
        auto proc = this_thread()->parent;
        auto pagemap = proc->pagemap;
        return pagemap->munmap(uintptr_t(addr), length) ? 0 : -1;
    }
} // namespace vmm