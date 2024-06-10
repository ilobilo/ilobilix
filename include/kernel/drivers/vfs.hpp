// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <lib/path.hpp>
#include <lib/time.hpp>

#include <unordered_map>
#include <cerrno>
#include <tuple>

namespace proc { struct process; }
namespace vfs
{
    struct filesystem;
    struct resource;
    struct fdhandle;
    struct node_t;

    inline bool stub_ref(resource *res);
    inline bool stub_unref(resource *res);

    #define return_err_func(err, ret...) { errno = err; return ret; }
    struct cdev_t
    {
        virtual bool open(resource *res, fdhandle *fd, proc::process *proc) return_err_func(ENOSYS, true);

        virtual ssize_t read(resource *res, fdhandle *fd, void *buffer, off_t offset, size_t count) return_err_func(ENOSYS, -1)
        virtual ssize_t write(resource *res, fdhandle *fd, const void *buffer, off_t offset, size_t count) return_err_func(ENOSYS, -1)

        virtual bool ref(resource *res, fdhandle *fd) { return stub_ref(res); }
        virtual bool unref(resource *res, fdhandle *fd) { return stub_unref(res); }

        virtual bool trunc(resource *res, fdhandle *fd, size_t length) return_err_func(ENOSYS, -1)

        virtual int ioctl(resource *res, fdhandle *fd, size_t request, uintptr_t argp) return_err_func(ENOTTY, -1)
        virtual void *mmap(resource *res, size_t fpage, int flags) return_err_func(ENOSYS, (void*)-1)
    };

    struct resource
    {
        std::unordered_map<std::string_view, node_t*> children;

        std::atomic<size_t> refcount;
        filesystem *fs;
        std::mutex lock;
        stat_t stat;

        cdev_t *cdev;

        inline bool open(fdhandle *fd, proc::process *proc) { return this->cdev->open(this, fd, proc); }

        inline ssize_t read(fdhandle *fd, void *buffer, off_t offset, size_t count)
            { return this->cdev->read(this, fd, buffer, offset, count); }

        inline ssize_t write(fdhandle *fd, const void *buffer, off_t offset, size_t count)
            { return this->cdev->write(this, fd, buffer, offset, count); }

        inline ssize_t read(void *buffer, off_t offset, size_t count)
            { return this->cdev->read(this, nullptr, buffer, offset, count); }

        inline ssize_t write(const void *buffer, off_t offset, size_t count)
            { return this->cdev->write(this, nullptr, buffer, offset, count); }

        inline bool ref(fdhandle *fd) { return this->cdev->ref(this, fd); }
        inline bool ref() { return this->cdev->ref(this, nullptr); }

        inline bool unref(fdhandle *fd) { return this->cdev->unref(this, fd); }
        inline bool unref() { return this->cdev->unref(this, nullptr); }

        inline bool trunc(fdhandle *fd, size_t length)
            { return this->cdev->trunc(this, fd, length); }

        inline int ioctl(fdhandle *fd, size_t request, uintptr_t argp)
            { return this->cdev->ioctl(this, fd, request, argp); }

        inline void *mmap(size_t fpage, int flags)
            { return this->cdev->mmap(this, fpage, flags); }

        resource(filesystem *fs, cdev_t *cdev) : refcount(1), fs(fs), lock(), cdev(cdev) { };
    };

    struct node_t
    {
        private:
        node_t *internal_reduce(bool symlinks, bool automount, size_t cnt);

        public:
        std::mutex lock;

        filesystem *fs;
        resource *res;

        std::string name;
        std::string target;

        node_t *mountgate;
        node_t *parent;

        node_t *reduce(bool symlinks, bool automount = true);
        path_t to_path();

        types type();
        mode_t mode();

        bool empty();

        node_t(std::string_view name) : name(name) { }
        node_t(std::string_view name, node_t *parent, filesystem *fs, resource *res = nullptr) : fs(fs), res(res), name(name), parent(parent) { }

        ~node_t()
        {
            if (this->res && this->res->refcount == 0 && this->res->stat.st_nlink == 0)
                delete this->res;
        }
    };

    struct filesystem
    {
        node_t *mounted_on = nullptr;
        void *mountdata = nullptr;
        node_t *root = nullptr;

        std::atomic<ino_t> inodes = 0;
        dev_t dev_id = 0;
        std::mutex lock;

        std::string name;

        filesystem(const std::string &name) : mounted_on(nullptr), mountdata(nullptr), root(nullptr), name(name) { }

        std::optional<std::string_view> get_value(std::string_view key);

        virtual node_t *mount(node_t *source, node_t *parent, std::string_view name, void *data) return_err_func(EINVAL, nullptr)
        virtual bool unmount() return_err_func(EINVAL, false)

        virtual bool populate(node_t *node, std::string_view single = "") return_err_func(EINVAL, false)
        virtual bool sync(resource *res) return_err_func(EINVAL, false)

        virtual node_t *create(node_t *parent, std::string_view name, mode_t mode) return_err_func(EINVAL, nullptr)
        virtual node_t *symlink(node_t *parent, std::string_view name, std::string_view target) return_err_func(EINVAL, nullptr)

        virtual node_t *link(node_t *parent, std::string_view name, node_t *old_node) return_err_func(EINVAL, nullptr)
        virtual bool unlink(node_t *node) return_err_func(EINVAL, false)

        virtual node_t *mknod(node_t *parent, std::string_view name, dev_t dev, mode_t mode) return_err_func(EINVAL, nullptr)

    };
    #undef return_err_func

    inline bool stub_ref(resource *res)
    {
        res->refcount++;
        return true;
    }

    inline bool stub_unref(resource *res)
    {
        res->refcount--;
        return true;
    }

    node_t *get_root();
    node_t *get_real(node_t *node);

    bool register_fs(filesystem *fs);
    filesystem *find_fs(std::string_view name);

    void recursive_delete(node_t *node, bool resources);

    std::tuple<node_t *, node_t *, std::string> path2node(node_t *parent, path_t path, bool automount = true);

    bool mount(node_t *parent, path_view_t source, path_view_t target, std::string_view fs_name, int flags = 0, void *data = nullptr);
    bool unmount(node_t *parent, path_view_t path, int flags = 0);

    node_t *create(node_t *parent, path_view_t path, mode_t mode);
    node_t *symlink(node_t *parent, path_view_t path, std::string_view target);

    node_t *link(node_t *old_parent, path_view_t old_path, node_t *new_parent, path_view_t new_path, int flags = 0);
    bool unlink(node_t *parent, path_view_t path, int flags = 0);

    stat_t *stat(node_t *parent, path_view_t path, int flags);
} // namespace vfs