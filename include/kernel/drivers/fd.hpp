// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <drivers/vfs.hpp>

namespace proc { struct process; }
namespace vfs
{
    struct fdhandle
    {
        lock_t lock;
        node_t *node;
        resource *res;

        std::vector<dirent*> dirents;
        bool dirents_invalid;

        std::atomic<size_t> refcount;
        off_t offset;
        int flags;
        bool dir;

        fdhandle(resource *res, int flags) : lock(), node(nullptr), res(res), dirents_invalid(false), refcount(1), offset(0), flags(flags), dir(false) { }

        bool generate_dirents();
    };

    struct fd
    {
        lock_t lock;
        fdhandle *handle;
        int flags;

        constexpr fd(fdhandle *handle, int flags) : handle(handle), flags(flags) { }
        constexpr fd(const fd &rhs) : handle(rhs.handle), flags(rhs.flags) { }
    };

    struct fd_table
    {
        private:
        bool internal_close_fd(int num);

        public:
        lock_t lock;
        std::unordered_map<size_t, fd*> fds;

        ~fd_table();

        bool close_fd(int num);
        int fd2num(fd *fd, int old_num, bool specific);
        int res2num(resource *res, int flags, int old_num, bool specific);

        fd *num2fd(int num);
    };

    fd *res2fd(resource *res, int flags);

    node_t *get_parent_dir(int dirfd, path_view_t path);
    std::optional<std::tuple<node_t*, node_t*, std::string>> fdpath2node(int dirfd, path_view_t path);
} // namespace vfs