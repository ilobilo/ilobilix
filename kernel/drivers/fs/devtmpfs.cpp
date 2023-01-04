// Copyright (C) 2022  ilobilo

#include <drivers/fs/dev/streams.hpp>
#include <drivers/fs/devtmpfs.hpp>
#include <lib/alloc.hpp>
#include <lib/misc.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

namespace devtmpfs
{
    static constexpr size_t default_size = pmm::page_size;
    vfs::filesystem *dev_fs = nullptr;
    vfs::node_t *dev_root = nullptr;

    struct resource : vfs::resource
    {
        size_t cap;
        uint8_t *data;

        ssize_t read(vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
        {
            lockit(this->lock);

            auto real_count = count;
            if (off_t(offset + count) >= this->stat.st_size)
                real_count = count - ((offset + count) - this->stat.st_size);

            memcpy(buffer, this->data + offset, real_count);
            return real_count;
        }

        ssize_t write(vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
        {
            lockit(this->lock);

            if (offset + count > this->cap)
            {
                auto new_cap = this->cap;
                while (offset + count >= new_cap)
                    new_cap *= 2;

                this->data = realloc(this->data, new_cap);
                this->cap = new_cap;
            }

            memcpy(this->data + offset, buffer, count);

            if (off_t(offset + count) >= this->stat.st_size)
            {
                this->stat.st_size = offset + count;
                this->stat.st_blocks = div_roundup(offset + count, this->stat.st_blksize);
            }
            return count;
        }

        bool trunc(vfs::fdhandle *fd, size_t length)
        {
            lockit(this->lock);

            if (length > this->cap)
            {
                auto new_cap = this->cap;
                while (new_cap < length)
                    new_cap *= 2;

                this->data = realloc(this->data, new_cap);
                this->cap = new_cap;
            }

            this->stat.st_size = off_t(length);
            this->stat.st_blocks = div_roundup(this->stat.st_size, this->stat.st_blksize);

            return true;
        }

        void *mmap(size_t fpage, int flags)
        {
            lockit(this->lock);

            void *ret = nullptr;
            if (flags & MAP_SHARED)
                ret = fromhh(this->data + fpage * pmm::page_size);
            else
            {
                ret = pmm::alloc();
                memcpy(tohh(ret), this->data + fpage * pmm::page_size, pmm::page_size);
            }

            return ret;
        }

        resource(devtmpfs *fs, mode_t mode) : vfs::resource(fs)
        {
            if ((mode & s_ifmt) == s_ifreg)
            {
                this->cap = default_size;
                this->data = malloc<uint8_t*>(this->cap);
                this->can_mmap = true;
            }

            this->stat.st_size = 0;
            this->stat.st_blocks = 0;
            this->stat.st_blksize = 512;
            this->stat.st_dev = fs->dev_id;
            this->stat.st_ino = fs->inodes++;
            this->stat.st_mode = mode;
            this->stat.st_nlink = 1;

            this->stat.st_atim = time::realtime;
            this->stat.st_mtim = time::realtime;
            this->stat.st_ctim = time::realtime;
        }

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

    // devtmpfs::mount() should only be called once
    vfs::node_t *devtmpfs::mount(vfs::node_t *, vfs::node_t *parent, std::string_view name, void *data)
    {
        if (dev_root == nullptr)
            dev_root = this->create(parent, "", 0755 | s_ifdir);

        return this->root = dev_root;
    }

    bool devtmpfs::unmount()
    {
        if (this->mountdata != nullptr)
            free(this->mountdata);

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

    vfs::node_t *add_dev(path_view_t path, vfs::resource *res)
    {
        static lock_t lock;
        lockit(lock);

        auto [nparent, node, basename] = vfs::path2node(dev_root, path);

        if (node != nullptr)
        {
            errno = EEXIST;
            return nullptr;
        }

        if (nparent == nullptr)
            return nullptr;

        node = new vfs::node_t(basename, nparent, dev_fs, res);

        res->stat.st_dev = dev_fs->dev_id;
        res->stat.st_ino = dev_fs->inodes++;
        res->stat.st_nlink = 1;

        lockit(nparent->lock);
        nparent->children[node->name] = node;

        return node;
    }

    void init()
    {
        vfs::register_fs(dev_fs = new devtmpfs());

        // Create if it doesn't exist
        vfs::create(nullptr, "/dev", 0755 | s_ifdir);
        vfs::mount(nullptr, "", "/dev", "devtmpfs");

        streams::init();
    }
} // namespace devtmpfs