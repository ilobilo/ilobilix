// Copyright (C) 2022  ilobilo

#pragma once

#include <drivers/vfs.hpp>

namespace proc { struct process; }
namespace vfs
{
    struct handle
    {
        lock_t lock;
        node_t *node;
        resource *res;

        std::atomic<size_t> refcount;
        off_t offset;
        int flags;
        bool dir;

        constexpr handle(resource *res, int flags) : lock(), node(nullptr), res(res), refcount(1), offset(0), flags(flags & (~((o_creat | o_directory | o_excl | o_noctty | o_nofollow | o_trunc) | o_cloexec))), dir(false) { }
    };

    struct fd
    {
        handle *handle;
        int flags;
    };

    fd *res2fd(resource *res, int flags);
} // namespace vfs