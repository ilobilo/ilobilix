// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <lib/types.hpp>

namespace vfs
{
    ssize_t sys_read(int fdnum, void *buffer, size_t count);
    ssize_t sys_write(int fdnum, void *buffer, size_t count);

    int sys_openat(int dirfd, const char *pathname, int flags, mode_t mode);
    int sys_open(const char *pathname, int flags, mode_t mode);
    int sys_creat(const char *pathname, mode_t mode);

    int sys_close(int fdnum);

    int sys_fstatat(int dirfd, const char *pathname, stat_t *statbuf, int flags);
    int sys_stat(const char *pathname, stat_t *statbuf);
    int sys_fstat(int fdnum, stat_t *statbuf);
    int sys_lstat(const char *pathname, stat_t *statbuf);

    off_t sys_lseek(int fdnum, off_t new_offset, int whence);

    int sys_ioctl(int fdnum, size_t request, uintptr_t arg);
    int sys_fcntl(int fdnum, int cmd, uintptr_t arg);

    int sys_dup3(int oldfd, int newfd, int flags);
    int sys_dup2(int oldfd, int newfd);
    int sys_dup(int oldfd);

    char *sys_getcwd(char *buffer, size_t size);

    int sys_chdir(const char *path);
    int sys_fchdir(int fd);

    ssize_t sys_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
    ssize_t sys_readlink(const char *pathname, char *buf, size_t bufsiz);

    int sys_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
    int sys_link(const char *oldpath, const char *newpath);

    int sys_unlinkat(int dirfd, const char *pathname, int flags);
    int sys_unlink(const char *pathname);

    int sys_fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);
    int sys_fchmod(int fdnum, mode_t mode);
    int sys_chmod(const char *pathname, mode_t mode);

    int sys_mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev);
    int sys_mknod(const char *pathname, mode_t mode, dev_t dev);

    int sys_mkdirat(int dirfd, const char *pathname, mode_t mode);
    int sys_mkdir(const char *pathname, mode_t mode);

    [[clang::no_sanitize("alignment")]]
    ssize_t sys_getdents(unsigned int dirfd, dirent *dirp, unsigned int count);
    ssize_t sys_getdents64(int dirfd, void *dirp, size_t count);

    int sys_chdir(const char *path);
    int sys_fchdir(int fd);
} // namespace vfs