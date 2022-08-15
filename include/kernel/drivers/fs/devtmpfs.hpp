// Copyright (C) 2022  ilobilo

#pragma once

#include <drivers/vfs.hpp>

namespace devtmpfs
{
    struct devtmpfs : vfs::filesystem
    {
        devtmpfs() : filesystem("devtmpfs") { }

        vfs::node_t *mount(vfs::node_t *source, vfs::node_t *parent, std::string_view name, void *data);
        // bool unmount();

        vfs::node_t *create(vfs::node_t *parent, std::string_view name, mode_t mode);
        vfs::node_t *symlink(vfs::node_t *parent, std::string_view name, std::string_view target);

        vfs::node_t *link(vfs::node_t *parent, std::string_view name, vfs::node_t *old_node);
        bool unlink(vfs::node_t *node);
    };
    extern vfs::filesystem *dev_fs;
    extern vfs::node_t *dev_root;

    vfs::node_t *add_dev(path_view_t path, vfs::resource *res);
    void init();
} // namespace devtmpfs