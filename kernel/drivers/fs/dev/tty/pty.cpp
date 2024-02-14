// Copyright (C) 2022-2024  ilobilo

#include <drivers/fs/dev/tty/pty.hpp>
#include <drivers/fs/devtmpfs.hpp>

namespace pty
{
    void init()
    {
        vfs::create(devtmpfs::dev_root, "pts", 0755 | s_ifdir);
    }
} // namespace pty