// Copyright (C) 2022-2024  ilobilo

#include <drivers/fs/devtmpfs.hpp>
#include <drivers/proc.hpp>
#include <drivers/vfs.hpp>
#include <lib/log.hpp>
#include <climits>

namespace vfs
{
    static std::unordered_map<std::string_view, filesystem*> filesystems;
    static node_t *root_node = new node_t("/");
    static std::mutex lock;

    node_t *get_root()
    {
        auto thread = this_thread();
        return thread ? thread->parent->root.get() : root_node;
    }

    node_t *get_real(node_t *node)
    {
        if (node != get_root()->reduce(true) && node->fs->mounted_on && node == node->fs->mounted_on->mountgate)
            return node->fs->mounted_on;
        return node;
    }

    node_t *node_t::internal_reduce(bool symlinks, bool automount, size_t cnt)
    {
        if (this->mountgate != nullptr && automount == true)
            return this->mountgate->internal_reduce(symlinks, automount, 0);

        if (this->target.empty() == false && symlinks == true)
        {
            if (cnt >= SYMLOOP_MAX - 1)
            {
                errno = ELOOP;
                return nullptr;
            }

            auto next_node = std::get<1>(path2node(this->parent, this->target, automount));
            if (next_node == nullptr)
            {
                errno = ENOENT;
                return nullptr;
            }

            return next_node->internal_reduce(symlinks, automount, ++cnt);
        }

        return this;
    }

    node_t *node_t::reduce(bool symlinks, bool automount)
    {
        return this->internal_reduce(symlinks, automount, 0);
    }

    path_t node_t::to_path()
    {
        std::string ret("");

        auto current = this;
        auto root = get_root();
        while (current != nullptr && current != root)
        {
            ret.insert(0, "/" + current->name);
            current = current->parent;
        }

        return ret.empty() ? ret += "/" : ret;
    }

    types node_t::type()
    {
        return this->res->stat.type();
    }

    mode_t node_t::mode()
    {
        return this->res->stat.mode();
    }

    bool node_t::empty()
    {
        this->fs->populate(this);
        return this->res->children.empty();
    }

    std::optional<std::string_view> filesystem::get_value(std::string_view key)
    {
        if (this->mountdata == nullptr)
            return std::nullopt;

        std::string_view datastr(static_cast<const char*>(this->mountdata));

        auto pos = datastr.find(key);
        if (pos != std::string::npos && datastr.at(pos + key.length()) == '=')
        {
            pos += key.length() + 1;
            auto end = datastr.find(',', pos);
            return datastr.substr(pos, end == std::string::npos ? end : end - pos);
        }

        return std::nullopt;
    }

    bool register_fs(filesystem *fs)
    {
        static std::mutex reg_lock;
        std::unique_lock guard(reg_lock);

        if (filesystems.contains(fs->name))
            return false;

        filesystems[fs->name] = fs;
        return true;
    }

    filesystem *find_fs(std::string_view name)
    {
        if (filesystems.contains(name))
            return filesystems[name];

        return nullptr;
    }

    void recursive_delete(node_t *node, bool resources)
    {
        if (node == nullptr)
            return;

        if (node->type() == s_ifdir)
            for (auto [name, child] : node->res->children)
                recursive_delete(child, resources);

        if (resources == true)
            delete node->res;

        delete node;
    }

    std::tuple<node_t *, node_t *, std::string> path2node(node_t *parent, path_t path, bool automount)
    {
        if (parent == nullptr || path.is_absolute())
            parent = get_root();

        auto current_node = parent->reduce(false);

        if (path == "/" || path.empty())
            return { current_node, current_node, "/" };

        auto get_parent = [&current_node]
        {
            auto parent = current_node->parent;

            if (current_node == get_root()->reduce(false))
                parent = current_node;
            else if (current_node == current_node->fs->root)
                parent = current_node->fs->mounted_on->parent;

            return parent;
        };

        for (const auto [segment, type, is_last, _] : path.segments())
        {
            if (type != CWK_NORMAL)
            {
                if (type == CWK_BACK)
                    current_node = get_parent();

                if (is_last == true)
                    return { get_parent(), current_node, current_node->name };

                continue;
            }

            if (current_node->res->children.contains(segment))
            {
                found:;
                auto node = current_node->res->children[segment]->reduce(false, is_last ? automount : true);

                if (is_last == true)
                    return { current_node, get_real(node), node->name };

                current_node = node;

                if (current_node->type() == s_iflnk)
                {
                    current_node = current_node->reduce(true);
                    if (current_node == nullptr)
                        return { nullptr, nullptr, "" };
                }

                if (current_node->type() != s_ifdir)
                {
                    errno = ENOTDIR;
                    return { nullptr, nullptr, "" };
                }

                continue;
            }
            else if (current_node->fs->populate(current_node, segment))
                goto found;

            errno = ENOENT;
            if (is_last == true)
                return { current_node, nullptr, { segment.data(), segment.length() } };

            return { nullptr, nullptr, "" };
        }

        errno = ENOENT;
        return { nullptr, nullptr, "" };
    }

    // TODO: flags
    bool mount(node_t *parent, path_view_t source, path_view_t target, std::string_view fs_name, int flags, void *data)
    {
        std::unique_lock guard(lock);

        auto fs = find_fs(fs_name);
        if (fs == nullptr)
        {
            errno = ENODEV;
            return false;
        }

        node_t *source_node = nullptr;
        if (source.empty() == false)
        {
            auto [sparent, snode, basename] = path2node(parent, source);
            if (snode == nullptr)
                return false;

            if (snode->type() != s_ifblk)
            {
                errno = ENOTBLK;
                return false;
            }

            source_node = snode;
        }

        auto [nparent, node, basename] = path2node(parent, target);
        bool is_root = (node == get_root());

        if (node == nullptr)
            return false;

        if (is_root == false && node->type() != s_ifdir)
        {
            errno = ENOTDIR;
            return false;
        }

        auto mountgate = fs->mount(source_node, nparent, basename, data);
        if (mountgate == nullptr)
            return false;

        node->mountgate = mountgate;
        mountgate->fs->mounted_on = node;

        if (source.empty())
            log::infoln("VFS: Mounted filesystem '{}' on '{}'", fs_name, target);
        else
            log::infoln("VFS: Mounted  '{}' on '{}' with filesystem '{}'", source, target, fs_name);

        return true;
    }

    // TODO: unmount, flags
    bool unmount(node_t *parent, path_view_t path, int flags);

    node_t *create(node_t *parent, path_view_t path, mode_t mode)
    {
        std::unique_lock guard(lock);

        auto [nparent, node, basename] = path2node(parent, path);
        if (node != nullptr)
            return_err(nullptr, EEXIST);

        if (nparent == nullptr)
            return nullptr;

        node = nparent->fs->create(nparent, basename, mode);
        if (node != nullptr)
        {
            std::unique_lock guard(nparent->lock);
            nparent->res->children[node->name] = node;
        }
        return node;
    }

    node_t *symlink(node_t *parent, path_view_t path, std::string_view target)
    {
        std::unique_lock guard(lock);

        auto [nparent, node, basename] = path2node(parent, path);
        if (node != nullptr)
            return_err(nullptr, EEXIST);

        if (nparent == nullptr)
            return nullptr;

        node = nparent->fs->symlink(nparent, basename, target);
        if (node != nullptr)
        {
            std::unique_lock guard(nparent->lock);
            nparent->res->children[node->name] = node;
        }
        return node;
    }

    node_t *link(node_t *old_parent, path_view_t old_path, node_t *new_parent, path_view_t new_path, int flags)
    {
        auto [old_tparent, old_node, old_basename] = path2node(old_parent, old_path);
        if (old_node == nullptr)
            return nullptr;

        if (flags & at_symlink_follow)
            old_node = old_node->reduce(true);

        if (old_node == nullptr)
            return nullptr;

        auto [new_tparent, new_node, new_basename] = path2node(new_parent ?: old_tparent, new_path);
        if (new_tparent == nullptr)
            return nullptr;

        if (new_node != nullptr)
            return_err(nullptr, EEXIST);

        if (old_tparent->fs != new_tparent->fs)
        {
            errno = EXDEV;
            return nullptr;
        }

        new_node = new_tparent->fs->link(new_tparent, new_basename, old_node);
        if (new_node != nullptr)
        {
            std::unique_lock guard(new_tparent->lock);

            // TODO: ???
            new_node->res->ref();
            new_node->res->stat.st_nlink++;

            new_tparent->res->children[new_node->name] = new_node;
        }

        return new_node;
    }

    bool unlink(node_t *parent, path_view_t path, int flags)
    {
        std::unique_lock guard(lock);

        auto [nparent, node, basename] = path2node(parent, path, false);

        if (node == nullptr)
            return false;

        if (node->mountgate != nullptr)
            return_err(false, EBUSY);

        if (node->type() == s_ifdir)
        {
            if (not (flags & at_removedir))
            {
                errno = EISDIR;
                return false;
            }

            if (node->empty() == false)
            {
                errno = ENOTEMPTY;
                return false;
            }
        }

        bool ret = nparent->fs->unlink(node);
        if (ret == true)
        {
            std::unique_lock guard(node->lock);

            // TODO: ???
            node->res->unref();
            node->res->stat.st_nlink--;

            delete node;
            nparent->res->children.erase(basename);
        }

        return ret;
    }

    stat_t *stat(node_t *parent, path_view_t path, int flags)
    {
        std::unique_lock guard(lock);

        bool automount = not (flags & at_no_automount);
        auto [nparent, node, basename] = path2node(parent, path, automount);

        if (node == nullptr)
            return nullptr;

        bool follow_symlinks = (flags & at_symlink_follow || not (flags & at_symlink_nofollow));
        node = node->reduce(follow_symlinks, automount);

        if (node == nullptr)
            return nullptr;

        node = get_real(node);

        return &node->res->stat;
    }
} // namespace vfs