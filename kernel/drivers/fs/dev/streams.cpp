// Copyright (C) 2022-2023  ilobilo

#include <drivers/fs/devtmpfs.hpp>
#include <init/kernel.hpp>
#include <random>

namespace streams
{
    struct null_cdev : vfs::cdev_t
    {
        ssize_t read(vfs::resource *res, vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
        {
            return 0;
        }

        ssize_t write(vfs::resource *res, vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
        {
            return count;
        }
    };

    struct full_cdev : vfs::cdev_t
    {
        ssize_t read(vfs::resource *res, vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
        {
            memset(buffer, 0, count);
            return count;
        }

        ssize_t write(vfs::resource *res, vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
        {
            errno = ENOSPC;
            return -1;
        }
    };

    struct zero_cdev : vfs::cdev_t
    {
        ssize_t read(vfs::resource *res, vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
        {
            memset(buffer, 0, count);
            return count;
        }

        ssize_t write(vfs::resource *res, vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
        {
            return count;
        }
    };

    struct random_cdev : vfs::cdev_t
    {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        std::mt19937_64 rng;
        smart_lock lock;

        random_cdev(uint64_t seed) : dist(0, 0xFF), rng(seed) { }

        ssize_t read(vfs::resource *res, vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
        {
            lockit(this->lock);

            auto u8buffer = static_cast<uint8_t*>(buffer);
            for (size_t i = 0; i < count; i++)
                u8buffer[i] = this->dist(this->rng);

            return count;
        }

        ssize_t write(vfs::resource *res, vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
        {
            return count;
        }
    };

    void init()
    {
        devtmpfs::register_dev(new null_cdev, makedev(1, 3));
        devtmpfs::register_dev(new full_cdev, makedev(1, 7));
        devtmpfs::register_dev(new zero_cdev, makedev(1, 5));

        // TODO: better way to get random seed?
        auto rcdev = new random_cdev(boot_time_request.response->boot_time);
        devtmpfs::register_dev(rcdev, makedev(1, 8));
        devtmpfs::register_dev(rcdev, makedev(1, 9));

        devtmpfs::add_dev("null", makedev(1, 3), 0666 | s_ifchr);
        devtmpfs::add_dev("full", makedev(1, 7), 0666 | s_ifchr);
        devtmpfs::add_dev("zero", makedev(1, 5), 0666 | s_ifchr);

        devtmpfs::add_dev("random", makedev(1, 8), 0666 | s_ifchr);
        devtmpfs::add_dev("urandom", makedev(1, 9), 0666 | s_ifchr);
    }
} // namespace streams