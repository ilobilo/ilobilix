// Copyright (C) 2022  ilobilo

#include <drivers/proc.hpp>
#include <drivers/fd.hpp>

// This code is **inspired** by lyre-os

namespace proc
{
    static bool internal_close_fd(process *proc, size_t num)
    {
        if (proc->fds.contains(num) == false)
        {
            errno = EBADF;
            return false;
        }
        auto fd = proc->fds[num];
        fd->handle->res->unref(fd->handle);

        if (fd->handle->refcount-- == 1)
            delete fd->handle;
        delete fd;

        proc->fds.erase(num);
        return true;
    }

    bool process::close_fd(size_t num)
    {
        lockit(this->fd_lock);
        return internal_close_fd(this, num);
    }

    size_t process::fd2num(vfs::fd *fd, size_t old_num, bool specific)
    {
        lockit(this->fd_lock);

        if (specific == false)
        {
            for (size_t i = old_num; i < std::numeric_limits<size_t>::max(); i++)
            {
                if (this->fds.contains(i) == false)
                {
                    this->fds[i] = fd;
                    return i;
                }
            }
        }
        else
        {
            internal_close_fd(this, old_num);
            this->fds[old_num] = fd;
            return old_num;
        }
        return -1;
    }

    size_t process::res2num(vfs::resource *res, int flags, size_t old_num, bool specific)
    {
        auto fd = res2fd(res, flags);
        if (fd == nullptr)
            return -1;
        return this->fd2num(fd, old_num, specific);
    }

    size_t process::dupfd(size_t old_num, process *new_proc, size_t new_num, int flags, bool specific, bool cloexec)
    {
        if (new_proc == nullptr)
            new_proc = this;

        if (specific && old_num == new_num && new_proc == this)
        {
            errno = EINVAL;
            return -1;
        }

        auto old_fd = this->num2fd(old_num);
        if (old_fd == nullptr)
            return -1;

        auto new_fd = new vfs::fd(*old_fd);
        new_num = new_proc->fd2num(new_fd, new_num, specific);
        if (new_num < 0)
        {
            delete new_fd;
            return -1;
        }

        new_fd->flags = flags;
        if (cloexec == true)
            new_fd->flags &= o_cloexec;

        old_fd->handle->refcount++;
        old_fd->handle->res->refcount++;

        return new_num;
    }

    vfs::fd *process::num2fd(size_t num)
    {
        lockit(this->fd_lock);

        if (this->fds.contains(num) == false)
        {
            errno = EBADF;
            return nullptr;
        }
        auto fd = this->fds[num];
        fd->handle->refcount++;
        return fd;
    }
} // namespace proc

namespace vfs
{
    vfs::fd *res2fd(vfs::resource *res, int flags)
    {
        res->refcount++;
        return new vfs::fd
        {
            new vfs::handle(res, flags),
            flags & o_cloexec
        };
    }
} // namespace vfs