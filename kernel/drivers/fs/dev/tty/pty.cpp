// Copyright (C) 2022-2023  ilobilo

#include <drivers/fs/dev/tty/pty.hpp>
#include <drivers/fs/devtmpfs.hpp>

namespace pty
{
    void init()
    {
        vfs::create(devtmpfs::dev_root, "pts", 0755 | s_ifdir);
    }
} // namespace pty