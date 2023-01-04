namespace vfs
{
    uintptr_t sys_read(int fdnum, void *buffer, size_t count)
    {
        auto proc = this_thread()->parent;

        auto fd = proc->num2fd(fdnum);
        if (fd == nullptr)
            return -1;

        auto handle = fd->handle;
        auto res = handle->res;

        if (res->stat.type() == s_ifdir)
            return_err(-1, EISDIR);

        auto ret = res->read(handle, buffer, handle->offset, count);
        if (ret < 0)
            return -1;

        res->stat.st_atim = time::realtime;

        handle->offset += ret;
        return ret;
    }

    uintptr_t sys_write(int fdnum, void *buffer, size_t count)
    {
        auto proc = this_thread()->parent;

        auto fd = proc->num2fd(fdnum);
        if (fd == nullptr)
            return -1;

        auto handle = fd->handle;
        auto res = handle->res;

        auto ret = res->write(handle, buffer, handle->offset, count);
        if (ret < 0)
            return -1;

        res->stat.st_mtim = res->stat.st_ctim = time::realtime;

        handle->offset += ret;
        return ret;
    }

    static node_t *get_parent_dir(int dirfd, path_view_t path)
    {
        auto proc = this_thread()->parent;

        if (path.is_absolute())
            return get_root();
        else if (dirfd == at_fdcwd)
            return proc->cwd;

        auto fd = proc->num2fd(dirfd);
        if (fd == nullptr)
            return nullptr;

        if (fd->handle->res->stat.type() != s_ifdir)
            return_err(nullptr, ENOTDIR);

        return fd->handle->node;
    }

    static std::optional<std::tuple<node_t*, node_t*, std::string>> fdpath2node(int dirfd, path_view_t path)
    {
        auto parent = get_parent_dir(dirfd, path);
        if (parent == nullptr)
            return std::nullopt;
        return path2node(parent, path);
    }

    uintptr_t sys_openat(int dirfd, const char *pathname, int flags, mode_t mode)
    {
        auto cflags = flags & file_creation_flags;
        auto follow = !(flags & o_nofollow);
        bool write = ((flags & o_accmode) == o_wronly || (flags & o_accmode) == o_rdwr);

        if (flags & o_tmpfile && (!(flags & o_wronly) && !(flags & o_rdwr)))
            return_err(-1, EINVAL);

        auto proc = this_thread()->parent;

        auto res = fdpath2node(dirfd, pathname);
        if (res.has_value() == false)
            return -1;

        auto parent = std::get<0>(res.value());
        if (parent == nullptr)
            return_err(-1, ENOENT);

        auto node = std::get<1>(path2node(parent, pathname));
        if (node == nullptr)
        {
            if (errno == ELOOP)
                return_err(-1, ELOOP);

            if (cflags & o_creat)
                node = create(parent, pathname, (mode & ~proc->umask) | s_ifreg);
            else
                return_err(-1, ENOENT);
        }
        else if (flags & o_creat && flags & o_excl)
            return_err(-1, EEXIST);

        node = node->reduce(follow);

        if (node == nullptr)
            return -1;

        if (node->res->stat.type() == s_iflnk)
            return_err(-1, ELOOP);

        node = node->reduce(true);
        if (node == nullptr)
            return -1;

        if (node->res->stat.type() == s_ifdir && write == true)
            return_err(-1, EISDIR);

        if (node->res->stat.type() != s_ifdir && flags & o_directory)
            return_err(-1, ENOTDIR);

        auto fd = res2fd(node->res, flags);
        if (flags & o_trunc && write == true)
            if (node->res->trunc(fd->handle, 0))
                node->res->stat.st_mtim = node->res->stat.st_ctim = time::realtime;

        node->res->stat.st_atim = time::realtime;
        fd->handle->node = node;
        fd->flags = (flags & o_cloexec) ? fd_cloexec : 0;
        return proc->fd2num(fd, 0, false);
    }

    uintptr_t sys_open(const char *pathname, int flags, mode_t mode)
    {
        return sys_openat(at_fdcwd, pathname, flags, mode);
    }

    uintptr_t sys_close(int fdnum)
    {
        auto proc = this_thread()->parent;
        if (proc->close_fd(fdnum) == false)
            return -1;
        return 0;
    }

    uintptr_t sys_fstatat(int dirfd, const char *pathname, stat_t *statbuf, int flags)
    {
        if (statbuf == nullptr)
            return_err(-1, EINVAL);

        auto proc = this_thread()->parent;

        if (strlen(pathname) == 0)
        {
            if (!(flags & at_empty_path))
                return_err(-1, ENOENT);

            if (dirfd == at_fdcwd)
                *statbuf = proc->cwd->res->stat;
            else
            {
                auto fd = proc->num2fd(dirfd);
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

    uintptr_t sys_stat(const char *pathname, stat_t *statbuf)
    {
        return sys_fstatat(at_fdcwd, pathname, statbuf, at_symlink_follow);
    }

    uintptr_t sys_fstat(int fdnum, stat_t *statbuf)
    {
        return sys_fstatat(fdnum, "", statbuf, at_empty_path | at_symlink_follow);
    }

    uintptr_t sys_lstat(const char *pathname, stat_t *statbuf)
    {
        return sys_fstatat(at_fdcwd, pathname, statbuf, at_symlink_nofollow);
    }

    uintptr_t sys_lseek(int fdnum, off_t new_offset, int whence)
    {
        auto proc = this_thread()->parent;

        auto fd = proc->num2fd(fdnum);
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
        {
            if (offset < 0)
                return_err(-1, EOVERFLOW);
        }

        fd->handle->offset = offset;
        return offset;
    }

    uintptr_t sys_ioctl(int fdnum, size_t request, uintptr_t arg)
    {
        auto proc = this_thread()->parent;

        auto fd = proc->num2fd(fdnum);
        if (fd == nullptr)
            return -1;

        return fd->handle->res->ioctl(fd->handle, request, arg);
    }

    uintptr_t sys_dup3(int oldfd, int newfd, int flags)
    {
        return this_thread()->parent->dupfd(oldfd, newfd, flags, true, flags & o_cloexec);
    }

    uintptr_t sys_dup2(int oldfd, int newfd)
    {
        return sys_dup3(oldfd, newfd, 0);
    }

    uintptr_t sys_dup(int oldfd)
    {
        return this_thread()->parent->dupfd(oldfd, 0, 0, false, false);
    }

    uintptr_t sys_getcwd(char *buffer, size_t size)
    {
        std::string cwd = this_thread()->parent->cwd->to_path();
        size_t len = cwd.length();

        if (size == 0)
        {
            if (buffer != nullptr)
                return_err(-1, EINVAL);
            size = len + 1;
        }
        else if (size - 1 < len)
            return_err(-1, ERANGE);

        if (buffer == nullptr)
            buffer = new char[len + 1];

        cwd.copy(buffer, len);
        return buffer[len] = 0;
    }

    uintptr_t sys_fcntl(int fdnum, int cmd, uintptr_t arg)
    {
        auto proc = this_thread()->parent;

        auto fd = proc->num2fd(fdnum);
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
                lockit(fd->lock);
                return fd->flags;
            }
            case f_setfd:
            {
                lockit(fd->lock);
                fd->flags = arg;
                return 0;
            }
            case f_getfl:
            {
                lockit(fd->handle->lock);
                return fd->handle->flags;
            }
            case f_setfl:
            {
                lockit(fd->handle->lock);
                fd->handle->flags = arg;
                return 0;
            }
        }

        // log::errorln("Unknown command: {}", cmd);
        return_err(-1, EINVAL);
    }

    ssize_t sys_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz)
    {
        auto parent = get_parent_dir(dirfd, pathname);
        if (parent == nullptr)
            return -1;

        auto node = std::get<1>(path2node(parent, pathname));
        if (node == nullptr)
            return -1;

        if (node->res->stat.type() != s_iflnk)
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
        // TODO
        return -1;
    }

    int sys_fchmod(int fdnum, mode_t mode)
    {
        return sys_fchmodat(fdnum, "", mode, 0);
    }

    int sys_chmod(const char *pathname, mode_t mode)
    {
        return sys_fchmodat(at_fdcwd, pathname, mode, 0);
    }
} // namespace vfs