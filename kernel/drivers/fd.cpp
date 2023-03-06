// Copyright (C) 2022-2023  ilobilo

#include <drivers/fs/devtmpfs.hpp>
#include <drivers/proc.hpp>
#include <drivers/fd.hpp>

namespace proc
{
    int process::dupfd(int old_num, process *new_proc, int new_num, int flags, bool specific, bool cloexec)
    {
        if (new_proc == nullptr)
            new_proc = this;

        if (specific && old_num == new_num && new_proc == this)
            return_err(-1, EINVAL);

        auto old_fd = this->fd_table->num2fd(old_num);
        if (old_fd == nullptr)
            return -1;

        auto new_fd = new vfs::fd(*old_fd);
        new_num = new_proc->fd_table->fd2num(new_fd, new_num, specific);
        if (new_num < 0)
        {
            delete new_fd;
            return -1;
        }

        new_fd->flags = flags;
        if (cloexec == false)
            new_fd->flags &= ~fd_cloexec;
        else
            new_fd->flags |= fd_cloexec;

        old_fd->handle->refcount++;
        old_fd->handle->res->ref();

        return new_num;
    }

    int process::open(int dirfd, std::string_view pathname, int flags, mode_t mode, int spec_fd)
    {
        if ((flags & o_accmode) == 0)
            flags |= o_rdonly;

        mode &= (s_irwxu | s_irwxg | s_irwxo | s_isvtx | s_isuid | s_isgid);

        auto follow = !(flags & o_nofollow);

        auto acc = flags & o_accmode;
        size_t accmode = 0;
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

        if (flags & o_tmpfile && !(accmode & stat_t::write))
            return_err(-1, EINVAL);

        auto ret = vfs::fdpath2node(dirfd, pathname);
        if (ret.has_value() == false)
            return -1;

        auto [parent, _, basename] = ret.value();
        if (parent == nullptr)
            return_err(-1, ENOENT);

        auto node = std::get<1>(vfs::path2node(parent, basename));
        if (node == nullptr)
        {
            if (errno == ELOOP)
                return_err(-1, ELOOP);

            if (flags & o_creat)
            {
                if (parent->res->stat.has_access(this->euid, this->egid, stat_t::write | stat_t::exec) == false)
                    return_err(-1, EACCES);
                node = vfs::create(parent, basename, (mode & ~this->umask.get()) | s_ifreg);
                node->res->stat.st_uid = this->euid;
                if (parent->res->stat.st_mode & s_isgid)
                    node->res->stat.st_gid = parent->res->stat.st_gid;
                else
                    node->res->stat.st_gid = this->egid;
            }
            else return_err(-1, ENOENT);
        }
        else if (flags & o_creat && flags & o_excl)
            return_err(-1, EEXIST);

        node = node->reduce(follow);

        if (node == nullptr)
            return -1;

        if (node->type() == s_iflnk)
            return_err(-1, ELOOP);

        if (node->type() == s_ifdir && accmode & stat_t::write)
            return_err(-1, EISDIR);

        if (node->type() != s_ifdir && flags & o_directory)
            return_err(-1, ENOTDIR);

        if (node->res->stat.has_access(this->euid, this->egid, accmode) == false)
            return_err(-1, EACCES);

        auto fdesc = res2fd(node->res, flags & ~o_cloexec);
        if (flags & o_trunc && accmode & stat_t::write)
            if (node->res->trunc(fdesc->handle, 0))
                node->res->stat.update_time(stat_t::modify | stat_t::status);

        node->res->stat.update_time(stat_t::access);
        fdesc->handle->node = node;

        if (node->res->open(fdesc->handle, this) == false)
        {
            delete fdesc->handle;
            delete fdesc;
            return -1;
        };

        return this->fd_table->fd2num(fdesc, std::max(spec_fd, 0), spec_fd >= 0);
    }
} // namespace proc

namespace vfs
{
    bool fdhandle::generate_dirents()
    {
        lockit(this->lock);
        if (this->node == nullptr || this->node->type() != s_ifdir)
            return_err(false, ENOTDIR);

        this->dirents.clear();
        for (const auto [name, child] : this->node->res->children)
        {
            auto cres = child->reduce(false)->res;
            auto reclen = dirent_len + name.length() + 1;

            auto ent = malloc<dirent*>(reclen);
            ent->d_ino = cres->stat.st_ino;
            ent->d_off = reclen;
            ent->d_reclen = reclen;
            ent->d_type = if2dt(cres->stat.st_mode);

            name.copy(ent->d_name, name.length());
            reinterpret_cast<char*>(ent->d_name)[name.length()] = 0;

            this->dirents.push_back(ent);
        }

        // . and .. are handled in path2node
        for (std::string_view name : { ".", ".." })
        {
            auto cres = name == "." ? this->node->res : (this->node->name == "/" ? this->node->res : this->node->parent->res);
            auto reclen = dirent_len + name.length() + 1;

            auto ent = malloc<dirent*>(reclen);
            ent->d_ino = cres->stat.st_ino;
            ent->d_off = reclen;
            ent->d_reclen = reclen;
            ent->d_type = if2dt(cres->stat.st_mode);

            name.copy(ent->d_name, name.length());
            reinterpret_cast<char*>(ent->d_name)[name.length()] = 0;

            this->dirents.push_back(ent);
        }
        return true;
    }

    fd_table::~fd_table()
    {
        for (auto [num, fd] : this->fds)
            this->close_fd(num);
    }

    bool fd_table::internal_close_fd(int num)
    {
        if (num < 0)
            return_err(false, EBADF);

        if (this->fds.contains(num) == false)
            return_err(false, EBADF);

        auto fd = this->fds[num];
        fd->handle->res->unref(fd->handle);

        if (fd->handle->refcount-- == 1)
            delete fd->handle;
        delete fd;

        this->fds.erase(num);
        return true;
    }

    bool fd_table::close_fd(int num)
    {
        lockit(this->lock);
        return this->internal_close_fd(num);
    }

    int fd_table::fd2num(fd *fd, int old_num, bool specific)
    {
        lockit(this->lock);

        if (old_num < 0)
            return_err(-1, EBADF);

        if (specific == true)
        {
            this->internal_close_fd(old_num);
            this->fds[old_num] = fd;
            return old_num;
        }

        for (auto i = old_num; i < std::numeric_limits<int>::max(); i++)
        {
            if (this->fds.contains(i) == false)
            {
                this->fds[i] = fd;
                return i;
            }
        }
        return_err(-1, EBADF);
    }

    int fd_table::res2num(resource *res, int flags, int old_num, bool specific)
    {
        auto fd = res2fd(res, flags);
        if (fd == nullptr)
            return -1;

        return this->fd2num(fd, old_num, specific);
    }

    fd *fd_table::num2fd(int num)
    {
        lockit(this->lock);

        if (num < 0)
            return_err(nullptr, EBADF);

        if (this->fds.contains(num) == false)
            return_err(nullptr, EBADF);

        auto fd = this->fds[num];
        fd->handle->refcount++;
        return fd;
    }

    fd *res2fd(resource *res, int flags)
    {
        res->ref();
        return new fd
        {
            new fdhandle(res, flags & ~(o_creat | o_directory | o_excl | o_noctty | o_nofollow | o_trunc | o_cloexec)),
            (flags & o_cloexec) ? fd_cloexec : 0
        };
    }

    node_t *get_parent_dir(int dirfd, path_view_t path)
    {
        auto proc = this_thread()->parent;

        if (path.is_absolute())
            return get_root();

        if (dirfd == at_fdcwd)
            return proc->cwd;

        auto fd = proc->fd_table->num2fd(dirfd);
        if (fd == nullptr)
            return nullptr;

        if (fd->handle->res->stat.type() != s_ifdir)
            return_err(nullptr, ENOTDIR);

        return fd->handle->node;
    }

    std::optional<std::tuple<node_t*, node_t*, std::string>> fdpath2node(int dirfd, path_view_t path)
    {
        auto parent = get_parent_dir(dirfd, path);
        if (parent == nullptr)
            return std::nullopt;
        return path2node(parent, path);
    }
} // namespace vfs