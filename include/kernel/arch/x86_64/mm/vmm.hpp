// Copyright (C) 2022  ilobilo

#pragma once

#include <mm/vmm.hpp>

namespace vmm
{
    enum x86_64_flags
    {
        Present = (1 << 0),
        Write = (1 << 1),
        UserSuper = (1 << 2),
        WriteThrough = (1 << 3), // PWT
        CacheDisable = (1 << 4), // PCD
        Accessed = (1 << 5),
        LargerPages = (1 << 7),
        PAT4k = (1 << 7), // PAT lvl1
        Custom0 = (1 << 9),
        Custom1 = (1 << 10),
        Custom2 = (1 << 11),
        PATlg = (1 << 12), // PAT lvl2+
        NoExec = (1UL << 63)
    };

    struct PDEntry
    {
        uint64_t value = 0;

        void setflags(uint64_t flags, bool enabled)
        {
            uint64_t bitSel = flags;

            auto temp = this->value;
            temp &= ~bitSel;
            if (enabled)
                temp |= bitSel;
            this->value = temp;
        }

        bool getflags(uint64_t flags)
        {
            return (this->value & flags) ? true : false;
        }

        uint64_t getaddr()
        {
            return (this->value & 0x000FFFFFFFFFF000) >> 12;
        }

        void setaddr(uint64_t address)
        {
            address &= 0x000000FFFFFFFFFF;

            auto temp = this->value;
            temp &= 0xFFF0000000000FFF;
            temp |= (address << 12);
            this->value = temp;
        }
    };

    struct [[gnu::aligned(0x1000)]] PTable
    {
        PDEntry entries[512];
    };

} // namespace vmm