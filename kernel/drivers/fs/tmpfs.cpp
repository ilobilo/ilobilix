// Copyright (C) 2022-2024  ilobilo

#include <drivers/fs/devtmpfs.hpp>
#include <drivers/fs/tmpfs.hpp>
#include <drivers/proc.hpp>
#include <lib/alloc.hpp>
#include <lib/misc.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

namespace tmpfs
{
    static constexpr size_t default_size = pmm::page_size;

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

        resource(tmpfs *fs, mode_t mode, vfs::cdev_t *cdev) : vfs::resource(fs, cdev)
        {
            if (mode2type(mode) == s_ifreg)
            {
                this->cap = default_size;
                this->data = malloc<uint8_t*>(this->cap);
                fs->current_size += this->cap;
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
        resource(tmpfs *fs, mode_t mode) : resource(fs, mode, new cdev_t) { }

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
        std::unique_lock guard(res->lock);

        auto real_count = count;
        if (off_t(offset + count) >= res->stat.st_size)
            real_count = count - ((offset + count) - res->stat.st_size);

        memcpy(buffer, res->data + offset, real_count);
        return real_count;
    }

    ssize_t cdev_t::write(vfs::resource *_res, vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
    {
        auto res = static_cast<resource*>(_res);
        std::unique_lock guard(res->lock);

        if (offset + count > res->cap)
        {
            auto new_cap = res->cap;
            while (offset + count >= new_cap)
                new_cap *= 2;

            auto tfs = static_cast<tmpfs*>(res->fs);
            if (tfs->current_size + (new_cap - res->cap) > tfs->max_size)
            {
                errno = ENOSPC;
                return -1;
            }

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
        std::unique_lock guard(res->lock);

        if (length > res->cap)
        {
            auto new_cap = res->cap;
            while (new_cap < length)
                new_cap *= 2;

            auto tfs = static_cast<tmpfs*>(res->fs);
            if (tfs->current_size + (new_cap - res->cap) > tfs->max_size)
            {
                errno = ENOSPC;
                return -1;
            }

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
        std::unique_lock guard(res->lock);

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

    void tmpfs::parse_data()
    {
        if (auto size = this->get_value("size"); size.has_value())
        {
            size_t count = std::stoll(size.value());
            switch (size.value().back())
            {
                case 'g':
                    count *= 1024;
                    [[fallthrough]];
                case 'm':
                    count *= 1024;
                    [[fallthrough]];
                case 'k':
                    count *= 1024;
                    break;
                case '%':
                    count *= pmm::total() / 100;
                    break;
                default:
                    return;
            }
            this->max_size = count;
        }
        else if (auto blocks = this->get_value("nr_blocks"); blocks.has_value())
        {
            size_t count = std::stoll(blocks.value());
            switch (blocks.value().back())
            {
                case 'g':
                    count *= 1024;
                    [[fallthrough]];
                case 'm':
                    count *= 1024;
                    [[fallthrough]];
                case 'k':
                    count *= 1024;
                    break;
                default:
                    return;
            }
            this->max_size = count * pmm::page_size;
        }
        else this->max_size = pmm::total() / 2;

        if (auto inodes = this->get_value("nr_inodes"); inodes.has_value())
        {
            size_t count = std::stoll(inodes.value());
            switch (inodes.value().back())
            {
                case 'g':
                    count *= 1024;
                    [[fallthrough]];
                case 'm':
                    count *= 1024;
                    [[fallthrough]];
                case 'k':
                    count *= 1024;
                    break;
                default:
                    return;
            }
            this->max_inodes = count;
        }
        else this->max_inodes = pmm::total() / pmm::page_size / 2;

        if (auto mode = this->get_value("mode"); mode.has_value())
            this->root_mode = std::stoll(mode.value(), nullptr, 8);

        if (auto gid = this->get_value("gid"); gid.has_value())
            this->root_gid = std::stoll(gid.value(), nullptr);

        if (auto uid = this->get_value("uid"); uid.has_value())
            this->root_uid = std::stoll(uid.value(), nullptr);
    }

    vfs::node_t *tmpfs::mount(vfs::node_t *, vfs::node_t *parent, std::string_view name, void *data)
    {
        auto fs = new tmpfs();
        fs->mountdata = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data))) : nullptr;

        fs->parse_data();

        fs->root = fs->create(parent, name, fs->root_mode | s_ifdir);
        fs->root->res->stat.st_gid = fs->root_gid;
        fs->root->res->stat.st_uid = fs->root_uid;

        return fs->root;
    }

    bool tmpfs::unmount()
    {
        vfs::recursive_delete(this->root, true);

        if (this->mountdata != nullptr)
            free(this->mountdata);

        return true;
    }

    bool tmpfs::sync(vfs::resource *res)
    {
        return true;
    }

    vfs::node_t *tmpfs::create(vfs::node_t *parent, std::string_view name, mode_t mode)
    {
        if (this->inodes >= this->max_inodes || (mode2type(mode) == s_ifreg && this->current_size + default_size > this->max_size))
        {
            errno = ENOSPC;
            return nullptr;
        }

        return new vfs::node_t(name, parent, this, new resource(this, mode));
    }

    vfs::node_t *tmpfs::symlink(vfs::node_t *parent, std::string_view name, std::string_view target)
    {
        if (this->inodes >= this->max_inodes)
        {
            errno = ENOSPC;
            return nullptr;
        }

        auto node = new vfs::node_t(name, parent, this, new resource(this, 0777 | s_iflnk));
        node->target = target;
        return node;
    }

    vfs::node_t *tmpfs::link(vfs::node_t *parent, std::string_view name, vfs::node_t *old_node)
    {
        if (old_node->type() == s_ifdir)
        {
            errno = EISDIR;
            return nullptr;
        }

        return new vfs::node_t(name, parent, this, old_node->res);
    }

    bool tmpfs::unlink(vfs::node_t *)
    {
        return true;
    }

    vfs::node_t *tmpfs::mknod(vfs::node_t *parent, std::string_view name, dev_t dev, mode_t mode)
    {
        auto cdev = devtmpfs::get_dev(dev);
        if (cdev == nullptr)
            return nullptr;

        auto res = new resource(this, mode, cdev);
        res->stat.st_rdev = dev;

        return new vfs::node_t(name, parent, this, res);
    }

    void init()
    {
        vfs::register_fs(new tmpfs());
    }
} // namespace tmpfs