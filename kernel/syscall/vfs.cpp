// Copyright (C) 2022-2023  ilobilo

#include <drivers/fs/devtmpfs.hpp>
#include <drivers/proc.hpp>
#include <syscall/vfs.hpp>
#include <lib/log.hpp>

namespace vfs
{
    static inline ssize_t flags2acc(int flags)
    {
        auto acc = flags & o_accmode;
        ssize_t accmode = 0;
        switch (acc)
        {
            case o_rdonly:
                accmode = stat_t::read;
                break;
            case o_wronly:
                accmode = stat_t::write;
                break;
            case o_rdwr:
                accmode = stat_t::read | stat_t::write;
                break;
            default:
                return_err(-1, EINVAL);
        }
        return accmode;
    }

    ssize_t sys_read(int fdnum, void *buffer, size_t count)
    {
        auto proc = this_thread()->parent;

        auto fd = proc->fd_table->num2fd(fdnum);
        if (fd == nullptr)
            return -1;

        auto handle = fd->handle;
        auto res = handle->res;

        if (res->stat.type() == s_ifdir)
            return_err(-1, EISDIR);

        if (!(flags2acc(handle->flags) & stat_t::read))
            return_err(-1, EBADF);

        auto ret = res->read(handle, buffer, handle->offset, count);
        res->stat.update_time(stat_t::access);

        if (res->fs->sync(res) == false || ret < 0)
            return -1;

        handle->offset += ret;
        return ret;
    }

    ssize_t sys_write(int fdnum, void *buffer, size_t count)
    {
        auto proc = this_thread()->parent;

        auto fd = proc->fd_table->num2fd(fdnum);
        if (fd == nullptr)
            return -1;

        auto handle = fd->handle;
        auto res = handle->res;

        if (!(flags2acc(handle->flags) & stat_t::write))
            return_err(-1, EBADF);

        if ((handle->flags & o_append) && res->stat.type() != s_ififo)
            handle->offset = res->stat.st_size;

        auto ret = res->write(handle, buffer, handle->offset, count);
        if (ret < 0)
            return -1;

        res->stat.update_time(stat_t::modify | stat_t::status);
        if (res->fs->sync(res) == false)
            return -1;

        handle->offset += ret;
        return ret;
    }

    int sys_openat(int dirfd, const char *pathname, int flags, mode_t mode)
    {
        return this_thread()->parent->open(dirfd, pathname, flags, mode);
    }

    int sys_open(const char *pathname, int flags, mode_t mode)
    {
        return sys_openat(at_fdcwd, pathname, flags, mode);
    }

    int sys_creat(const char *pathname, mode_t mode)
    {
        return sys_open(pathname, o_creat | o_wronly | o_trunc, mode);
    }

    int sys_close(int fdnum)
    {
        auto proc = this_thread()->parent;
        if (proc->fd_table->close_fd(fdnum) == false)
            return -1;
        return 0;
    }

    int sys_fstatat(int dirfd, const char *pathname, stat_t *statbuf, int flags)
    {
        if (statbuf == nullptr)
            return_err(-1, EINVAL);

        auto proc = this_thread()->parent;

        if (strlen(pathname) == 0)
        {
            if (!(flags & at_empty_path))
                return_err(-1, ENOENT);

            if (dirfd == at_fdcwd)
                *statbuf = proc->cwd.get()->res->stat;
            else
            {
                auto fd = proc->fd_table->num2fd(dirfd);
                if (fd == nullptr)
                    return -1;
                *statbuf = fd->handle->res->stat;
            }
        }
        else
        {
            auto parent = get_parent_dir(dirfd, pathname);
            if (parent == nullptr)
                return -1;

            auto val = stat(parent, pathname, flags);
            if (val.has_value() == false)
                return -1;

            *statbuf = val.value();
        }
        return 0;
    }

    int sys_stat(const char *pathname, stat_t *statbuf)
    {
        return sys_fstatat(at_fdcwd, pathname, statbuf, at_symlink_follow);
    }

    int sys_fstat(int fdnum, stat_t *statbuf)
    {
        return sys_fstatat(fdnum, "", statbuf, at_empty_path | at_symlink_follow);
    }

    int sys_lstat(const char *pathname, stat_t *statbuf)
    {
        return sys_fstatat(at_fdcwd, pathname, statbuf, at_symlink_nofollow);
    }

    off_t sys_lseek(int fdnum, off_t new_offset, int whence)
    {
        auto proc = this_thread()->parent;

        auto fd = proc->fd_table->num2fd(fdnum);
        if (fd == nullptr)
            return -1;

        auto type = fd->handle->res->stat.type();
        if (type == s_ifchr || type == s_ifsock || type == s_ififo)
            return_err(-1, ESPIPE);

        auto offset = fd->handle->offset;
        switch (whence)
        {
            case seek_set:
                offset = new_offset;
                break;
            case seek_cur:
                if (offset + new_offset > std::numeric_limits<off_t>::max())
                    return_err(-1, EOVERFLOW);
                offset += new_offset;
                break;
            case seek_end:
            {
                auto size = fd->handle->res->stat.st_size;
                if (size + new_offset > std::numeric_limits<off_t>::max())
                    return_err(-1, EOVERFLOW);
                offset = size + new_offset;
                break;
            }
            default:
                return_err(-1, EINVAL);
        }

        if constexpr (std::is_signed_v<off_t>)
            if (offset < 0)
                return_err(-1, EOVERFLOW);

        fd->handle->offset = offset;
        return offset;
    }

    int sys_ioctl(int fdnum, size_t request, uintptr_t arg)
    {
        auto proc = this_thread()->parent;

        auto fd = proc->fd_table->num2fd(fdnum);
        if (fd == nullptr)
            return -1;

        return fd->handle->res->ioctl(fd->handle, request, arg);
    }

    int sys_fcntl(int fdnum, int cmd, uintptr_t arg)
    {
        auto proc = this_thread()->parent;

        auto fd = proc->fd_table->num2fd(fdnum);
        if (fd == nullptr)
            return -1;

        switch (cmd)
        {
            case f_dupfd:
                return proc->dupfd(fdnum, arg, 0, false, false);
            case f_dupfd_cloexec:
                return proc->dupfd(fdnum, arg, 0, false, true);
            case f_getfd:
            {
                std::unique_lock guard(fd->lock);
                return fd->flags;
            }
            case f_setfd:
            {
                std::unique_lock guard(fd->lock);
                fd->flags = arg;
                return 0;
            }
            case f_getfl:
            {
                std::unique_lock guard(fd->handle->lock);
                return fd->handle->flags;
            }
            case f_setfl:
            {
                if (arg & o_accmode)
                    return_err(-1, EINVAL);
                std::unique_lock guard(fd->handle->lock);
                fd->handle->flags = arg;
                return 0;
            }
        }

        log::errorln("Unknown command: {}", cmd);
        return_err(-1, EINVAL);
    }

    int sys_dup3(int oldfd, int newfd, int flags)
    {
        return this_thread()->parent->dupfd(oldfd, newfd, flags, true, flags & o_cloexec);
    }

    int sys_dup2(int oldfd, int newfd)
    {
        return sys_dup3(oldfd, newfd, 0);
    }

    int sys_dup(int oldfd)
    {
        return this_thread()->parent->dupfd(oldfd, 0, 0, false, false);
    }

    char *sys_getcwd(char *buffer, size_t size)
    {
        std::string cwd = this_thread()->parent->cwd.get()->to_path();
        size_t len = cwd.length();

        if (size == 0)
        {
            if (buffer != nullptr)
                return_err(nullptr, EINVAL);
            size = len + 1;
        }
        else if (size - 1 < len)
            return_err(nullptr, ERANGE);

        if (buffer == nullptr)
            buffer = new char[len + 1];

        cwd.copy(buffer, len);
        buffer[len] = 0;
        return buffer;
    }

    int sys_chdir(const char *path)
    {
        auto thread = this_thread();
        auto proc = thread->parent;

        if (path == nullptr)
            return_err(-1, EINVAL);
        if (strlen(path) == 0)
            return_err(-1, ENOENT);

        auto node = std::get<1>(vfs::path2node(proc->cwd, path));
        if (node == nullptr)
            return -1;

        node = node->reduce(true);
        if (node == nullptr)
            return -1;

        if (node->res->stat.type() != s_ifdir)
            return_err(-1, ENOTDIR);

        proc->cwd = node;
        return 0;
    }

    int sys_fchdir(int fdnum)
    {
        auto thread = this_thread();
        auto proc = thread->parent;

        auto fd = proc->fd_table->num2fd(fdnum);
        if (fd == nullptr)
            return -1;

        auto node = fd->handle->node;
        if (node->res->stat.type() != s_ifdir)
            return_err(-1, ENOTDIR);

        proc->cwd = node;
        return 0;
    }

    ssize_t sys_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz)
    {
        auto res = fdpath2node(dirfd, pathname);
        if (res.has_value() == false)
            return -1;

        auto [parent, _, basename] = res.value();
        if (parent == nullptr)
            return_err(-1, ENOENT);

        auto node = std::get<1>(path2node(parent, basename));
        if (node == nullptr)
            return -1;

        if (node->type() != s_iflnk)
            return_err(-1, EINVAL);

        node = node->reduce(true);
        if (node == nullptr)
            return -1;

        std::string path = node->to_path();
        return path.copy(buf, bufsiz);
    }

    ssize_t sys_readlink(const char *pathname, char *buf, size_t bufsiz)
    {
        return sys_readlinkat(at_fdcwd, pathname, buf, bufsiz);
    }

    int sys_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags)
    {
        auto old_parent = get_parent_dir(olddirfd, oldpath);
        if (old_parent == nullptr)
            return -1;

        auto new_parent = get_parent_dir(newdirfd, newpath);
        if (new_parent == nullptr)
            return -1;

        return link(old_parent, oldpath, new_parent, newpath, flags) ? 0 : -1;
    }

    int sys_link(const char *oldpath, const char *newpath)
    {
        return sys_linkat(at_fdcwd, oldpath, at_fdcwd, newpath, 0);
    }

    int sys_unlinkat(int dirfd, const char *pathname, int flags)
    {
        auto parent = get_parent_dir(dirfd, pathname);
        if (parent == nullptr)
            return -1;

        return unlink(parent, pathname, flags) ? 0 : -1;
    }

    int sys_unlink(const char *pathname)
    {
        return sys_unlinkat(at_fdcwd, pathname, 0);
    }

    int sys_fchmodat(int dirfd, const char *pathname, mode_t mode, int flags)
    {
        auto proc = this_thread()->parent;

        auto ret = fdpath2node(dirfd, pathname);
        if (ret.has_value() == false)
            return -1;

        auto [parent, node, basename] = ret.value();
        if (node == nullptr)
            return_err(-1, ENOENT);

        node = node->reduce(!(flags & at_symlink_nofollow));
        if (node == nullptr)
            return_err(-1, ENOENT);

        if (node->res->stat.has_access(proc->uid, proc->gid, flags2acc(flags)) == false)
            return_err(-1, EACCES);

        if (proc->euid != node->res->stat.st_uid || proc->egid != node->res->stat.st_gid)
            return_err(-1, EPERM);

        auto mask = (s_irwxu | s_irwxg | s_irwxo | s_isuid | s_isgid | s_isvtx);
        node->res->stat.st_mode &= ~mask;
        node->res->stat.st_mode |= mode & mask;
        node->res->stat.update_time(stat_t::status);

        return node->fs->sync(node->res) ? 0 : -1;
    }

    int sys_fchmod(int fdnum, mode_t mode)
    {
        return sys_fchmodat(fdnum, "", mode, 0);
    }

    int sys_chmod(const char *pathname, mode_t mode)
    {
        return sys_fchmodat(at_fdcwd, pathname, mode, 0);
    }

    int sys_mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev)
    {
        auto type = mode2type(mode);
        switch (type)
        {
            case s_ififo:
            case s_ifsock:
                log::errorln("mknod: s_ififo and s_ifsock are not yet supported!");
                return_err(-1, EINVAL);
            case s_ifdir:
            case s_iflnk:
                return_err(-1, EINVAL);
            default:
                break;
        }

        auto proc = this_thread()->parent;

        auto parent = get_parent_dir(dirfd, pathname);
        if (parent == nullptr)
            return -1;

        auto node = (type == s_ifreg) ?
            devtmpfs::mknod(parent, pathname, dev, (mode & ~proc->umask.get()) | type) :
            create(parent, pathname, mode);

        if (node != nullptr)
        {
            node->res->stat.st_uid = proc->euid;
            if (parent->res->stat.st_mode & s_isgid)
                node->res->stat.st_gid = parent->res->stat.st_gid;
            else
                node->res->stat.st_gid = proc->egid;
            return 0;
        }
        return -1;
    }

    int sys_mknod(const char *pathname, mode_t mode, dev_t dev)
    {
        return sys_mknodat(at_fdcwd, pathname, mode, dev);
    }

    int sys_mkdirat(int dirfd, const char *pathname, mode_t mode)
    {
        auto proc = this_thread()->parent;
        mode = (mode & ~proc->umask.get() & 0777);

        auto parent = get_parent_dir(dirfd, pathname);
        if (parent == nullptr)
            return -1;

        auto node = create(parent, pathname, mode | s_ifdir);
        if (node != nullptr)
        {
            node->res->stat.st_uid = proc->euid;
            if (parent->res->stat.st_mode & s_isgid)
            {
                node->res->stat.st_gid = parent->res->stat.st_gid;
                node->res->stat.st_mode |= s_isgid;
            }
            else node->res->stat.st_gid = proc->egid;
            return 0;
        }
        return -1;
    }

    int sys_mkdir(const char *pathname, mode_t mode)
    {
        return sys_mkdirat(at_fdcwd, pathname, mode);
    }

    [[clang::no_sanitize("alignment")]]
    ssize_t sys_getdents(unsigned int dirfd, dirent *dirp, unsigned int count)
    {
        auto proc = this_thread()->parent;

        auto fd = proc->fd_table->num2fd(dirfd);
        if (fd == nullptr)
            return -1;

        auto handle = fd->handle;
        if (handle->dirents.empty())
            handle->generate_dirents();

        if (handle->dirents.empty())
            return 0;

        size_t length = 0;
        for (const auto &ent : handle->dirents)
            length += ent->d_reclen;

        length = std::min(size_t(count), length);

        if (handle->dirents.front()->d_reclen > count)
            return_err(-1, EINVAL);

        bool end = handle->dirents_invalid;
        handle->dirents_invalid = false;

        size_t i = 0;
        size_t bytes = 0;
        while (i < length)
        {
            auto ent = handle->dirents.pop_front_element();
            memcpy(reinterpret_cast<char*>(dirp) + i, ent, ent->d_reclen);
            bytes = i;
            i += ent->d_reclen;
            free(ent);
        }

        if (handle->dirents.empty())
            handle->dirents_invalid = true;

        return end ? 0 : bytes;
    }

    ssize_t sys_getdents64(int dirfd, void *dirp, size_t count)
    {
        return sys_getdents(dirfd, static_cast<dirent*>(dirp), count);
    }
} // namespace vfs