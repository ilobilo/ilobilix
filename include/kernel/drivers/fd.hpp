// Copyright (C) 2022  ilobilo

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

        std::atomic<size_t> refcount;
        off_t offset;
        int flags;
        bool dir;

        constexpr fdhandle(resource *res, int flags) : lock(), node(nullptr), res(res), refcount(1), offset(0), flags(flags & file_status_flags), dir(false) { }
    };

    struct fd
    {
        lock_t lock;
        fdhandle *handle;
        int flags;

        constexpr fd(fdhandle *handle, int flags) : handle(handle), flags(flags) { }
        constexpr fd(const fd &rhs) : handle(rhs.handle), flags(rhs.flags) { }
    };

    fd *res2fd(resource *res, int flags);
} // namespace vfs