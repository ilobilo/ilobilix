// Copyright (C) 2022  ilobilo

#pragma once

#define SYMLOOP_MAX 40

using ssize_t = long;
using off_t = long;

using dev_t = unsigned long;
using ino_t = unsigned long;

using nlink_t = unsigned long;
using mode_t = unsigned int;

using blksize_t = long;
using blkcnt_t = long;

using uid_t = unsigned int;
using gid_t = unsigned int;

using pid_t = int;
using tid_t = int;

using time_t = long;
using clockid_t = int;

enum prctls
{
    arch_set_gs = 0x1001,
    arch_set_fs = 0x1002,
    arch_get_fs = 0x1003,
    arch_get_gs = 0x1004
};

enum seeks
{
    seek_set = 0,
    seek_cur = 1,
    seek_end = 2
};

enum fcntls
{
    f_dupfd_cloexec = 1030,
    f_dupfd = 0,
    f_getfd = 1,
    f_setfd = 2,
    f_getfl = 3,
    f_setfl = 4,

    f_setown = 8,
    f_getown = 9,
    f_setsig = 10,
    f_getsig = 11,

    fd_cloexec = 1
};

enum types
{
    s_ifmt = 0170000,
    s_ifsock = 0140000,
    s_iflnk = 0120000,
    s_ifreg = 0100000,
    s_ifblk = 0060000,
    s_ifdir = 0040000,
    s_ifchr = 0020000,
    s_ififo = 0010000,
};

enum dtypes
{
    dt_unknown = 0,
    dt_fifo = 1,
    dt_chr = 2,
    dt_dir = 4,
    dt_blk = 6,
    dt_reg = 8,
    dt_lnk = 10,
    dt_sock = 12,
    dt_wht = 14
};

enum modes
{
    s_irwxu = 00700, // user rwx
    s_irusr = 00400, // user r
    s_iwusr = 00200, // user w
    s_ixusr = 00100, // user x
    s_irwxg = 00070, // group rwx
    s_irgrp = 00040, // group r
    s_iwgrp = 00020, // group w
    s_ixgrp = 00010, // group x
    s_irwxo = 00007, // others rwx
    s_iroth = 00004, // others r
    s_iwoth = 00002, // others w
    s_ixoth = 00001, // others x
    s_isuid = 04000, // set-user-id
    s_isgid = 02000, // set-group-id
    s_isvtx = 01000, // set-sticky
};

enum oflags
{
    o_async = 020000,
    o_direct = 040000,
    o_largefile = 0100000,
    o_noatime = 01000000,
    o_path = 010000000,
    o_tmpfile = 020000000,
    o_exec = o_path,
    o_search = o_path,

    o_accmode = (03 | o_path),
    o_rdonly = 00,
    o_wronly = 01,
    o_rdwr = 02,

    o_creat = 0100,
    o_excl = 0200,
    o_noctty = 0400,
    o_trunc = 01000,
    o_append = 02000,
    o_nonblock = 04000,
    o_dsync = 010000,
    o_sync = 04010000,
    o_rsync = 04010000,
    o_directory = 0200000,
    o_nofollow = 0400000,
    o_cloexec = 02000000,

    file_creation_flags = o_creat | o_directory | o_excl | o_noctty | o_nofollow | o_trunc,
    file_descriptor_flags = fd_cloexec,
    file_status_flags = ~(file_creation_flags | file_descriptor_flags)
};

enum atflags
{
    at_fdcwd = -100,
    at_symlink_follow = 0x400,
    at_symlink_nofollow = 0x100,
    at_removedir = 0x200,
    at_no_automount = 0x800,
    at_empty_path = 0x1000,
    at_eaccess = 0x200
};

struct timespec
{
    time_t tv_sec;
    long tv_nsec;

    constexpr timespec() : tv_sec(0), tv_nsec(0) { }
    constexpr timespec(time_t s, long ns) : tv_sec(s), tv_nsec(ns) { }
    constexpr timespec(time_t ms) : tv_sec(ms / 1'000), tv_nsec((ms % 1'000) * 1'000'000) { }

    constexpr long to_ns()
    {
        return this->tv_sec * 1'000'000'000 + this->tv_nsec;
    }

    constexpr time_t to_ms()
    {
        return this->to_ns() / 1'000'000;
    }

    constexpr timespec &operator+=(const timespec &rhs)
    {
        this->tv_sec += rhs.tv_sec;
        this->tv_nsec += rhs.tv_nsec;

        if (this->tv_nsec >= 1'000'000'000)
        {
            this->tv_sec++;
            this->tv_nsec -= 1'000'000'000;
        }

        return *this;
    }

    friend constexpr timespec operator+(const timespec &lhs, const timespec &rhs)
    {
        timespec ret = lhs;
        return ret += rhs;
    }

    friend constexpr timespec operator+(const timespec &lhs, long ns)
    {
        return lhs + timespec(0, ns);
    }

    constexpr timespec &operator-=(const timespec &rhs)
    {
        this->tv_sec -= rhs.tv_sec;
        this->tv_nsec -= rhs.tv_nsec;

        if (this->tv_nsec < 0)
        {
            this->tv_sec--;
            this->tv_nsec += 1'000'000'000;
        }
        if (this->tv_sec < 0)
        {
            this->tv_sec = 0;
            this->tv_nsec = 0;
        }

        return *this;
    }

    friend constexpr timespec operator-(const timespec &lhs, const timespec &rhs)
    {
        timespec ret = lhs;
        return ret -= rhs;
    }

    friend constexpr timespec operator-(const timespec &lhs, long ns)
    {
        return lhs - timespec(0, ns);
    }
};

inline constexpr unsigned char if2dt(mode_t mode)
{
    return (mode & s_ifmt) >> 12;
}

inline constexpr mode_t dt2if(unsigned char dt)
{
    return dt << 12;
}

struct stat_t
{
    dev_t st_dev;
    ino_t st_ino;
    nlink_t st_nlink;
    mode_t st_mode;
    uid_t st_uid;
    gid_t st_gid;
    unsigned int __unused0;
    dev_t st_rdev;
    off_t st_size;

    blksize_t st_blksize;
    blkcnt_t st_blocks;

    timespec st_atim;
    timespec st_mtim;
    timespec st_ctim;

    #define st_atime st_atim.tv_sec
    #define st_mtime st_mtim.tv_sec
    #define st_ctime st_ctim.tv_sec

    long __unused1[3];

    constexpr types type()
    {
        return types(this->st_mode & s_ifmt);
    }

    constexpr mode_t mode()
    {
        return this->st_mode & ~s_ifmt;
    }
};

inline constexpr unsigned int major(dev_t dev)
{
    unsigned int ret = ((dev & dev_t(0x00000000000FFF00U)) >> 8);
    return ret |= ((dev & dev_t(0xFFFFF00000000000U)) >> 32);
}

inline constexpr unsigned int minor(dev_t dev)
{
    unsigned int ret = (dev & dev_t(0x00000000000000FFU));
    return ret |= ((dev & dev_t(0x00000FFFFFF00000U)) >> 12);
}

inline constexpr dev_t makedev(unsigned int maj, unsigned int min)
{
    dev_t ret  = dev_t(maj & 0x00000FFFU) << 8;
    ret |= dev_t(maj & 0xFFFFF000U) << 32;
    ret |= dev_t(min & 0x000000FFU);
    ret |= dev_t(min & 0xFFFFFF00U) << 12;
    return ret;
}

struct dirent
{
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};

struct utsname
{
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};