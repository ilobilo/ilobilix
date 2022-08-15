// Copyright (C) 2022  ilobilo

#pragma once

#include <drivers/vfs.hpp>

namespace tmpfs
{
    struct tmpfs : vfs::filesystem
    {
        size_t max_inodes = 0;
        size_t max_size = 0;

        size_t current_size = 0;

        mode_t root_mode = 0;
        gid_t root_gid = 0;
        uid_t root_uid = 0;

        void parse_data();

        tmpfs() : filesystem("tmpfs"), max_inodes(0), max_size(0), current_size(0), root_mode(vfs::default_folder_mode), root_gid(0), root_uid(0) { }

        vfs::node_t *mount(vfs::node_t *source, vfs::node_t *parent, std::string_view name, void *data);
        // bool unmount();

        vfs::node_t *create(vfs::node_t *parent, std::string_view name, mode_t mode);
        vfs::node_t *symlink(vfs::node_t *parent, std::string_view name, std::string_view target);

        vfs::node_t *link(vfs::node_t *parent, std::string_view name, vfs::node_t *old_node);
        bool unlink(vfs::node_t *node);
    };

    void init();
} // namespace tmpfs