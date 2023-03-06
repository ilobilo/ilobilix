// Copyright (C) 2022-2023  ilobilo

#include <drivers/initrd/ustar.hpp>
#include <drivers/fs/devtmpfs.hpp>
#include <drivers/vfs.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>

namespace ustar
{
    bool validate(uintptr_t address)
    {
        return strncmp(reinterpret_cast<header*>(address)->magic, TMAGIC, 6) == 0;
    }

    template<typename Type>
    inline Type oct2int(const char *str, size_t len)
    {
        Type value = 0;
        while (*str && len > 0)
        {
            value = value * 8 + (*str++ - '0');
            len--;
        }
        return value;
    }

    void init(uintptr_t address)
    {
        auto current = reinterpret_cast<header*>(address);
        while (!strncmp(current->magic, TMAGIC, 6))
        {
            std::string_view name(current->name);
            std::string_view target(current->linkname);

            auto mode = oct2int<mode_t>(current->mode, sizeof(current->mode));
            auto size = oct2int<size_t>(current->size, sizeof(current->size));
            auto mtim = oct2int<time_t>(current->mtime, sizeof(current->mtime));

            auto devmajor = oct2int<time_t>(current->devmajor, sizeof(current->devmajor));
            auto devminor = oct2int<time_t>(current->devminor, sizeof(current->devminor));

            vfs::node_t *node = nullptr;

            if (name == "./")
                goto next;

            switch (current->typeflag)
            {
                case REGTYPE:
                case AREGTYPE:
                    node = vfs::create(nullptr, name, mode | s_ifreg);
                    if (node == nullptr)
                        log::errorln("USTAR: Could not create regular file '{}'", name);
                    else if (node->res->write(reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(current) + 512), 0, size) != ssize_t(size))
                        log::errorln("USTAR: Could not write to regular file '{}'", name);
                    break;
                case LNKTYPE:
                    node = vfs::link(nullptr, name, nullptr, target);
                    if (node == nullptr)
                        log::errorln("USTAR: Could not create hardlink '{}' -> '{}'", name, target);
                    break;
                case SYMTYPE:
                    node = vfs::symlink(nullptr, name, target);
                    if (node == nullptr)
                        log::errorln("USTAR: Could not create symlink '{}' -> '{}'", name, target);
                    break;
                case CHRTYPE:
                    devtmpfs::mknod(nullptr, name, makedev(devmajor, devminor), mode | s_ifchr);
                    if (node != nullptr)
                        log::errorln("USTAR: Could not create character file '{}' ({}:{})", name, devmajor, devminor);
                    break;
                case BLKTYPE:
                    log::errorln("USTAR: TODO: BLKDEV");
                    break;
                case DIRTYPE:
                    node = vfs::create(nullptr, name, mode | s_ifdir);
                    if (node == nullptr)
                        log::errorln("USTAR: Could not creare directory '{}'", name);
                    break;
                case FIFOTYPE:
                    log::errorln("USTAR: TODO: FIFO");
                    break;
            }

            if (node != nullptr)
                node->res->stat.st_mtim = timespec(mtim, 0);

            next:;
            current = reinterpret_cast<header*>(reinterpret_cast<uintptr_t>(current) + 512 + align_up(size, 512));
        }
    }
} // namespace ustar