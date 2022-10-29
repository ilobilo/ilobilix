// Copyright (C) 2022  ilobilo

#include <drivers/fs/tmpfs.hpp>
#include <lib/alloc.hpp>
#include <lib/misc.hpp>
#include <mm/pmm.hpp>

namespace tmpfs
{
    static constexpr size_t default_size = pmm::page_size;
    struct resource : vfs::resource
    {
        size_t cap;
        uint8_t *data;

        ssize_t read(void *buffer, off_t offset, size_t count)
        {
            lockit(this->lock);

            auto real_count = count;
            if (off_t(offset + count) >= this->stat.st_size)
                real_count = count - ((offset + count) - this->stat.st_size);

            memcpy(buffer, this->data + offset, real_count);
            return real_count;
        }

        ssize_t write(const void *buffer, off_t offset, size_t count)
        {
            lockit(this->lock);

            if (offset + count > this->cap)
            {
                auto new_cap = this->cap;
                while (offset + count >= new_cap)
                    new_cap *= 2;

                auto tfs = static_cast<tmpfs*>(this->fs);
                auto offset = new_cap - this->cap;
                if (tfs->current_size + offset > tfs->max_size)
                {
                    errno = ENOSPC;
                    return -1;
                }

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

        resource(tmpfs *fs, mode_t mode) : vfs::resource(fs)
        {
            if ((mode & s_ifmt) == s_ifreg)
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

    void tmpfs::parse_data()
    {
        if (auto size = this->get_value("size"); size.has_value())
        {
            size_t count = std::stoll(size.value(), nullptr);
            switch (size.value().back())
            {
                case 'k':
                    count *= 1024;
                    break;
                case 'm':
                    count *= 1024 * 1024;
                    break;
                case 'g':
                    count *= 1024 * 1024 * 1024;
                    break;
                case '%':
                    count *= pmm::total() / 100;
                    break;
            }
            this->max_size = count;
        }
        else if (auto blocks = this->get_value("nr_blocks"); blocks.has_value())
        {
            size_t count = std::stoll(blocks.value(), nullptr);
            switch (blocks.value().back())
            {
                case 'k':
                    count *= 1024;
                    break;
                case 'm':
                    count *= 1024 * 1024;
                    break;
                case 'g':
                    count *= 1024 * 1024 * 1024;
                    break;
            }
            this->max_size = count * pmm::page_size;
        }
        else this->max_size = pmm::total() / 100 * 50;

        if (auto inodes = this->get_value("nr_inodes"); inodes.has_value())
        {
            size_t count = std::stoll(inodes.value(), nullptr);
            switch (inodes.value().back())
            {
                case 'k':
                    count *= 1024;
                    break;
                case 'm':
                    count *= 1024 * 1024;
                    break;
                case 'g':
                    count *= 1024 * 1024 * 1024;
            }
            this->max_inodes = count;
        }
        else this->max_inodes = pmm::total() / pmm::page_size / 2;

        if (auto mode = this->get_value("mode"); mode.has_value())
        {
            size_t count = std::stoll(mode.value(), nullptr);
            this->root_mode = count;
        }
        else this->root_mode = vfs::default_folder_mode;

        if (auto gid = this->get_value("gid"); gid.has_value())
        {
            size_t count = std::stoll(gid.value(), nullptr);
            this->root_gid = count;
        }

        if (auto uid = this->get_value("uid"); uid.has_value())
        {
            size_t count = std::stoll(uid.value(), nullptr);
            this->root_uid = count;
        }
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

    // TODO: Is this correct?
    bool tmpfs::unmount()
    {
        vfs::recursive_delete(this->root, true);

        if (this->mountdata != nullptr)
            free(this->mountdata);

        return true;
    }

    vfs::node_t *tmpfs::create(vfs::node_t *parent, std::string_view name, mode_t mode)
    {
        if (this->inodes >= this->max_inodes || (types(mode & s_ifmt) == s_ifreg && this->current_size + default_size > this->max_size))
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

    void init()
    {
        vfs::register_fs(new tmpfs());
    }
} // namespace tmpfs