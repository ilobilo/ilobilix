// Copyright (C) 2022  ilobilo

#include <drivers/fs/devtmpfs.hpp>
#include <drivers/term.hpp>

namespace streams
{
    struct null : vfs::resource
    {
        ssize_t read(vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
        {
            return 0;
        }

        ssize_t write(vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
        {
            return count;
        }

        null() : vfs::resource(devtmpfs::dev_fs) { }
    };

    struct full : vfs::resource
    {
        ssize_t read(vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
        {
            memset(buffer, 0, count);
            return count;
        }

        ssize_t write(vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
        {
            errno = ENOSPC;
            return -1;
        }

        full() : vfs::resource(devtmpfs::dev_fs) { }
    };

    struct zero : vfs::resource
    {
        ssize_t read(vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
        {
            memset(buffer, 0, count);
            return count;
        }

        ssize_t write(vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
        {
            return count;
        }

        zero() : vfs::resource(devtmpfs::dev_fs) { }
    };

    // TMP start
    struct console : vfs::resource
    {
        ssize_t read(vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
        {
            time::msleep(1000);
            memset(buffer, '1', count);
            return count;
        }

        ssize_t write(vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
        {
            term::main_term->write(static_cast<const char *>(buffer), count);
            return count;
        }

        int ioctl(vfs::fdhandle *fd, size_t request, uintptr_t argp) { return 0; }

        console() : vfs::resource(devtmpfs::dev_fs) { }
    };
    // TMP end

    void init()
    {
        auto null_res = new null;
        null_res->stat.st_mode = vfs::default_file_mode | s_ifchr;
        null_res->stat.st_rdev = makedev(1, 3);
        devtmpfs::add_dev("null", null_res);

        auto full_res = new full;
        full_res->stat.st_mode = vfs::default_file_mode | s_ifchr;
        full_res->stat.st_rdev = makedev(1, 7);
        devtmpfs::add_dev("full", full_res);

        auto zero_res = new zero;
        zero_res->stat.st_mode = vfs::default_file_mode | s_ifchr;
        zero_res->stat.st_rdev = makedev(1, 5);
        devtmpfs::add_dev("zero", zero_res);

        // TMP start
        auto console_res = new console;
        console_res->stat.st_mode = vfs::default_file_mode | s_ifchr;
        console_res->stat.st_rdev = makedev(5, 1);
        devtmpfs::add_dev("console", console_res);
        // TMP end
    }
} // namespace streams