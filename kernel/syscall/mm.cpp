// Copyright (C) 2022-2023  ilobilo

#include <drivers/proc.hpp>
#include <syscall/mm.hpp>

namespace vmm
{
    void *sys_mmap(void *addr, size_t length, int prot, int flags, int fdnum, off_t offset)
    {
        auto proc = this_thread()->parent;
        auto pagemap = proc->pagemap;
        vfs::resource *res = nullptr;

        if (fdnum != -1)
        {
            auto fd = proc->fd_table->num2fd(fdnum);
            if (fd == nullptr)
                return mmap::map_failed;
            res = fd->handle->res;
        }
        else if (offset != 0)
            return_err(mmap::map_failed, EINVAL);

        return pagemap->mmap(uintptr_t(addr), length, prot, flags, res, offset);
    }

    int sys_mprotect(void *addr, size_t length, int prot)
    {
        auto proc = this_thread()->parent;
        auto pagemap = proc->pagemap;
        return pagemap->mprotect(uintptr_t(addr), length, prot) ? 0 : -1;
    }

    int sys_munmap(void *addr, size_t length)
    {
        auto proc = this_thread()->parent;
        auto pagemap = proc->pagemap;
        return pagemap->munmap(uintptr_t(addr), length) ? 0 : -1;
    }
} // namespace vmm