// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <drivers/vfs.hpp>

namespace devtmpfs
{
    struct devtmpfs : vfs::filesystem
    {
        devtmpfs() : filesystem("devtmpfs") { }

        vfs::node_t *mount(vfs::node_t *source, vfs::node_t *parent, std::string_view name, void *data);
        bool unmount();

        bool sync(vfs::resource *res);

        vfs::node_t *create(vfs::node_t *parent, std::string_view name, mode_t mode);
        vfs::node_t *symlink(vfs::node_t *parent, std::string_view name, std::string_view target);

        vfs::node_t *link(vfs::node_t *parent, std::string_view name, vfs::node_t *old_node);
        bool unlink(vfs::node_t *node);

        vfs::node_t *mknod(vfs::node_t *parent, std::string_view name, dev_t dev, mode_t mode);
    };

    extern vfs::filesystem *dev_fs;
    extern vfs::node_t *dev_root;

    extern std::unordered_map<dev_t, vfs::cdev_t*> devs;

    bool register_dev(vfs::cdev_t *cdev, dev_t dev);
    bool unregister_dev(dev_t dev);
    vfs::cdev_t *get_dev(dev_t dev);

    vfs::node_t *mknod(vfs::node_t *parent, path_view_t path, dev_t dev, mode_t mode);
    vfs::node_t *add_dev(path_view_t path, dev_t dev, mode_t mode);

    void init();
} // namespace devtmpfs