// Copyright (C) 2022-2023  ilobilo

#include <drivers/fs/dev/streams.hpp>
#include <drivers/fs/devtmpfs.hpp>
#include <drivers/proc.hpp>
#include <drivers/fd.hpp>
#include <lib/alloc.hpp>
#include <lib/misc.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <cstdlib>

namespace devtmpfs
{
    static constexpr size_t default_size = pmm::page_size;

    vfs::filesystem *dev_fs = nullptr;
    vfs::node_t *dev_root = nullptr;

    struct cdev_t : vfs::cdev_t
    {
        ssize_t read(vfs::resource *_res, vfs::fdhandle *fd, void *buffer, off_t offset, size_t count);
        ssize_t write(vfs::resource *_res, vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count);
        bool trunc(vfs::resource *_res, vfs::fdhandle *fd, size_t length);
        void *mmap(vfs::resource *_res, size_t fpage, int flags);
    };

    struct resource : vfs::resource
    {
        size_t cap;
        uint8_t *data;

        resource(devtmpfs *fs, mode_t mode, vfs::cdev_t *cdev) : vfs::resource(fs, cdev)
        {
            if ((mode & s_ifmt) == s_ifreg)
            {
                this->cap = default_size;
                this->data = malloc<uint8_t*>(this->cap);
            }

            this->stat.st_size = 0;
            this->stat.st_blocks = 0;
            this->stat.st_blksize = 512;
            this->stat.st_dev = fs->dev_id;
            this->stat.st_ino = fs->inodes++;
            this->stat.st_mode = mode;
            this->stat.st_nlink = 1;

            auto proc = this_thread()->parent;
            this->stat.st_uid = proc->euid;
            this->stat.st_gid = proc->egid;

            this->stat.update_time(stat_t::access | stat_t::modify | stat_t::status);
        }

        resource(devtmpfs *fs, mode_t mode) : resource(fs, mode, new cdev_t) { }

        ~resource()
        {
            if (this->data != nullptr)
            {
                this->cap = 0;
                this->stat.st_size = 0;
                this->stat.st_blocks = 0;
                free(this->data);
            }
        }
    };

    ssize_t cdev_t::read(vfs::resource *_res, vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
    {
        auto res = static_cast<resource*>(_res);
        lockit(res->lock);

        auto real_count = count;
        if (off_t(offset + count) >= res->stat.st_size)
            real_count = count - ((offset + count) - res->stat.st_size);

        memcpy(buffer, res->data + offset, real_count);
        return real_count;
    }

    ssize_t cdev_t::write(vfs::resource *_res, vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
    {
        auto res = static_cast<resource*>(_res);
        lockit(res->lock);

        if (offset + count > res->cap)
        {
            auto new_cap = res->cap;
            while (offset + count >= new_cap)
                new_cap *= 2;

            res->data = realloc(res->data, new_cap);
            res->cap = new_cap;
        }

        memcpy(res->data + offset, buffer, count);

        if (off_t(offset + count) >= res->stat.st_size)
        {
            res->stat.st_size = offset + count;
            res->stat.st_blocks = div_roundup(offset + count, res->stat.st_blksize);
        }
        return count;
    }

    bool cdev_t::trunc(vfs::resource *_res, vfs::fdhandle *fd, size_t length)
    {
        auto res = static_cast<resource*>(_res);
        lockit(res->lock);

        if (length > res->cap)
        {
            auto new_cap = res->cap;
            while (new_cap < length)
                new_cap *= 2;

            res->data = realloc(res->data, new_cap);
            res->cap = new_cap;
        }

        res->stat.st_size = off_t(length);
        res->stat.st_blocks = div_roundup(res->stat.st_size, res->stat.st_blksize);

        return true;
    }

    void *cdev_t::mmap(vfs::resource *_res, size_t fpage, int flags)
    {
        auto res = static_cast<resource*>(_res);
        lockit(res->lock);

        void *ret = nullptr;
        if (flags & vmm::mmap::map_shared)
            ret = fromhh(res->data + fpage * pmm::page_size);
        else
        {
            ret = pmm::alloc();
            memcpy(tohh(ret), res->data + fpage * pmm::page_size, pmm::page_size);
        }

        return ret;
    }

    vfs::node_t *devtmpfs::mount(vfs::node_t *, vfs::node_t *parent, std::string_view name, void *data)
    {
        if (dev_root == nullptr)
            dev_root = this->create(parent, "", 0755 | s_ifdir);

        return this->root = new vfs::node_t(name, parent, this, dev_root->res);
    }

    bool devtmpfs::unmount()
    {
        if (this->mountdata != nullptr)
            free(this->mountdata);

        return true;
    }

    bool devtmpfs::sync(vfs::resource *res)
    {
        return true;
    }

    vfs::node_t *devtmpfs::create(vfs::node_t *parent, std::string_view name, mode_t mode)
    {
        return new vfs::node_t(name, parent, this, new resource(this, mode));
    }

    vfs::node_t *devtmpfs::symlink(vfs::node_t *parent, std::string_view name, std::string_view target)
    {
        auto node = new vfs::node_t(name, parent, this, new resource(this, 0777 | s_iflnk));
        node->target = target;
        return node;
    }

    vfs::node_t *devtmpfs::link(vfs::node_t *parent, std::string_view name, vfs::node_t *old_node)
    {
        if (old_node->type() == s_ifdir)
        {
            errno = EISDIR;
            return nullptr;
        }

        return new vfs::node_t(name, parent, this, old_node->res);
    }

    bool devtmpfs::unlink(vfs::node_t *)
    {
        return true;
    }

    vfs::node_t *devtmpfs::mknod(vfs::node_t *parent, std::string_view name, dev_t dev, mode_t mode)
    {
        auto cdev = get_dev(dev);
        if (cdev == nullptr)
            return nullptr;

        auto res = new resource(this, mode, cdev);
        res->stat.st_rdev = dev;

        return new vfs::node_t(name, parent, this, res);
    }

    std::unordered_map<dev_t, vfs::cdev_t*> devs;
    lock_t lock;

    bool register_dev(vfs::cdev_t *cdev, dev_t dev)
    {
        lockit(lock);
        if (devs.contains(dev) == true)
            return false;
        devs[dev] = cdev;
        return true;
    }

    bool unregister_dev(dev_t dev)
    {
        lockit(lock);
        if (devs.contains(dev) == false)
            return false;
        devs.erase(dev);
        return true;
    }

    vfs::cdev_t *get_dev(dev_t dev)
    {
        if (devs.contains(dev) == false)
            return_err(nullptr, ENODEV);
        return devs[dev];
    }

    vfs::node_t *mknod(vfs::node_t *parent, path_view_t path, dev_t dev, mode_t mode)
    {
        lockit(lock);

        auto [nparent, node, basename] = vfs::path2node(parent, path);
        if (node != nullptr)
            return_err(nullptr, EEXIST);

        if (nparent == nullptr)
            return nullptr;

        lockit(nparent->lock);
        node = nparent->fs->mknod(nparent, basename, dev, mode);
        return nparent->res->children[node->name] = node;
    }

    vfs::node_t *add_dev(path_view_t path, dev_t dev, mode_t mode)
    {
        return mknod(dev_root, path, dev, mode);
    }

    void init()
    {
        vfs::register_fs(dev_fs = new devtmpfs());

        vfs::create(nullptr, "/dev", 0755 | s_ifdir);
        vfs::mount(nullptr, "", "/dev", "devtmpfs");

        streams::init();
    }
} // namespace devtmpfs