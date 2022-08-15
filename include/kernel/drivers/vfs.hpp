// Copyright (C) 2022  ilobilo

#pragma once

#include <unordered_map>
#include <lib/path.hpp>
#include <lib/time.hpp>
#include <lib/lock.hpp>
#include <cerrno>
#include <vector>
#include <memory>
#include <tuple>

namespace vfs
{
    static constexpr mode_t default_file_mode = 0666;
    static constexpr mode_t default_folder_mode = 0777;

    #define return_err_func(err, ret...) { errno = err; return ret; }
    struct filesystem;
    struct resource
    {
        std::atomic<size_t> refcount;
        filesystem *fs;
        lock_t lock;
        stat_t stat;

        virtual ssize_t read(void *buffer, off_t offset, size_t count) return_err_func(EINVAL, -1)
        virtual ssize_t write(const void *buffer, off_t offset, size_t count) return_err_func(EINVAL, -1)

        virtual int ioctl(uint64_t request, void *argp) return_err_func(EINVAL, -1)

        resource(filesystem *fs) : refcount(1), fs(fs), lock() { };
        virtual ~resource() = default;
    };

    struct node_t
    {
        private:
        node_t *internal_reduce(bool symlinks, bool automount, size_t cnt);

        public:
        lock_t lock;

        filesystem *fs;
        resource *res;

        std::string name;
        std::string target;

        node_t *mountgate;
        node_t *parent;
        std::unordered_map<std::string_view, node_t*> children;

        node_t *reduce(bool symlinks, bool automount = true);
        path_t to_path();

        std::optional<types> type();
        std::optional<mode_t> mode();

        bool dotentries();

        node_t(std::string_view name) : name(name) { }
        node_t(std::string_view name, node_t *parent, filesystem *fs, resource *res = nullptr) : fs(fs), res(res), name(name), parent(parent) { }
    };

    struct filesystem
    {
        node_t *mounted_on = nullptr;
        void *mountdata = nullptr;
        node_t *root = nullptr;

        std::atomic<ino_t> inodes = 0;
        dev_t dev_id = 0;
        lock_t lock;

        std::string name;

        filesystem(const std::string &name) : mounted_on(nullptr), mountdata(nullptr), root(nullptr), name(name) { }

        std::optional<std::string> get_value(std::string_view key);

        virtual node_t *mount(node_t *source, node_t *parent, std::string_view name, void *data) return_err_func(EINVAL, nullptr)
        virtual bool unmount() return_err_func(EINVAL, false)

        virtual void sync() return_err_func(EINVAL)

        virtual bool populate(node_t *node, std::string_view single = "") return_err_func(EINVAL, false)

        virtual node_t *create(node_t *parent, std::string_view name, mode_t mode) return_err_func(EINVAL, nullptr)
        virtual node_t *symlink(node_t *parent, std::string_view name, std::string_view target) return_err_func(EINVAL, nullptr)

        virtual node_t *link(node_t *parent, std::string_view name, node_t *old_node) return_err_func(EINVAL, nullptr)
        virtual bool unlink(node_t *node) return_err_func(EINVAL, false)

        virtual ssize_t read(node_t *node, void *buffer, off_t offset, size_t count) return_err_func(EINVAL, -1)
        virtual ssize_t write(node_t *node, const void *buffer, off_t offset, size_t count) return_err_func(EINVAL, -1)

        virtual int ioctl(node_t *node, uint64_t request, void *argp) return_err_func(EINVAL, -1)

    };
    #undef return_err_func

    node_t *get_root();

    bool register_fs(filesystem *fs);
    filesystem *find_fs(std::string_view name);

    std::tuple<node_t*, node_t*, std::string> path2node(node_t *parent, path_t path, bool automount = true);

    bool mount(node_t *parent, path_view_t source, path_view_t target, std::string_view fs_name, int flags = 0, void *data = nullptr);
    bool unmount(node_t *parent, path_view_t path, int flags = 0);

    node_t *create(node_t *parent, path_view_t path, mode_t mode);
    node_t *symlink(node_t *parent, path_view_t path, std::string_view target);

    node_t *link(node_t *parent, path_view_t new_path, path_view_t old_path, int flags = 0);
    bool unlink(node_t *parent, path_view_t path, int flags = 0);

    std::optional<stat_t> stat(node_t *parent, path_view_t path, int flags);
} // namespace vfs