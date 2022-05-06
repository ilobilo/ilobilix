// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__x86_64__) || defined(_M_X64)

#include <lib/lock.hpp>
#include <cstdint>

namespace arch::x86_64::vmm
{
    static constexpr uint64_t large_page_size = 0x200000;
    static constexpr uint64_t page_size = 0x1000;

    enum flags
    {
        Present = (1 << 0),
        ReadWrite = (1 << 1),
        UserSuper = (1 << 2),
        WriteThrough = (1 << 3),
        CacheDisable = (1 << 4),
        Accessed = (1 << 5),
        LargerPages = (1 << 7),
        PAT = (1 << 7),
        Custom0 = (1 << 9),
        Custom1 = (1 << 10),
        Custom2 = (1 << 11),
        NX = (1UL << 63)
    };

    struct PDEntry
    {
        uint64_t value = 0;

        void setflags(uint64_t flags, bool enabled)
        {
            uint64_t bitSel = flags;
            this->value &= ~bitSel;
            if (enabled) this->value |= bitSel;
        }
        bool getflags(uint64_t flags)
        {
            return (this->value & flags) ? true : false;
        }

        uint64_t getAddr()
        {
            return (this->value & 0x000FFFFFFFFFF000) >> 12;
        }
        void setAddr(uint64_t address)
        {
            address &= 0x000000FFFFFFFFFF;
            this->value &= 0xFFF0000000000FFF;
            this->value |= (address << 12);
        }
    };

    struct [[gnu::aligned(0x1000)]] PTable
    {
        PDEntry entries[512];
    };

    struct Pagemap
    {
        PTable *toplvl = nullptr;
        lock_t lock;

        PDEntry *virt2pte(uint64_t vaddr, bool allocate = true, bool hugepages = false);
        uint64_t virt2phys(uint64_t vaddr, bool hugepages = false)
        {
            PDEntry *pml_entry = this->virt2pte(vaddr, false, hugepages);
            if (pml_entry == nullptr || !pml_entry->getflags(Present)) return 0;

            return pml_entry->getAddr() << 12;
        }

        bool mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags = (Present | ReadWrite), bool hugepages = false);
        bool remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags = (Present | ReadWrite), bool hugepages = false);
        bool unmapMem(uint64_t vaddr, bool hugepages = false);

        void switchTo();
        void save();

        Pagemap(bool user = false);
    };

    extern bool lvl5;

    uint64_t getPagemap();

    void init();
} // namespace arch::x86_64::vmm

#endif