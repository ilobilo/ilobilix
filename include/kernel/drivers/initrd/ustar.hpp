// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

namespace ustar
{
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

    #define TMAGIC "ustar"
    #define TMAGLEN 6
    #define TVERSION "00"
    #define TVERSLEN 2

    #define REGTYPE '0'
    #define AREGTYPE '\0'
    #define LNKTYPE '1'
    #define SYMTYPE '2'
    #define CHRTYPE '3'
    #define BLKTYPE '4'
    #define DIRTYPE '5'
    #define FIFOTYPE '6'
    #define CONTTYPE '7'

    #define XHDTYPE 'x'
    #define XGLTYPE 'g'

    bool validate(uintptr_t address);
    void init(uintptr_t address);
} // namespace ustar