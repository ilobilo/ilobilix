// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <cstdint>

namespace ustar
{
    inline constexpr auto magic = "ustar";
    inline constexpr auto magic_len = 6;
    inline constexpr auto version = "00";
    inline constexpr auto version_len = 2;

    enum types : char
    {
        regular = '0',
        aregular = '\0',
        hardlink = '1',
        symlink = '2',
        chardev = '3',
        blockdev = '4',
        directory = '5',
        fifo = '6',
        control = '7',
        xhd = 'x',
        xgl = 'g'
    };

    struct [[gnu::packed]] header
    {
        char name[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char chksum[8];
        char typeflag;
        char linkname[100];
        char magic[6];
        char version[2];
        char uname[32];
        char gname[32];
        char devmajor[8];
        char devminor[8];
        char prefix[155];
    };

    bool validate(uintptr_t address);
    void init(uintptr_t address);
} // namespace ustar