// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <cstdint>

namespace mm::vmm
{
    enum flags
    {
        READ = (1 << 0),
        WRITE = (1 << 1),
        EXEC = (1 << 2),
        USER = (1 << 3),
        LPAGES = (1 << 4),

        RW = READ | WRITE,
        RWX = READ | WRITE | EXEC,

        RWU = RW | USER,
        RWXU = RWX | USER
    };

    enum caching
    {
        UNCACHABLE,
        WRITE_COMBINING,
        WRITE_THROUGH,
        WRITE_PROTECTED,
        WRITE_BACK,

        MMIO = UNCACHABLE
    };

    static constexpr caching default_caching = WRITE_BACK;
    static constexpr uint64_t default_flags = RWX;

    struct Pagemap
    {
        void *toplvl = nullptr;
        uint64_t large_page_size = 0;
        uint64_t page_size = 0;
        lock_t lock;

        uint64_t virt2phys(uint64_t vaddr, bool largepages = false);
        bool mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags = default_flags, caching cache = default_caching);
        bool remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags = default_flags, caching cache = default_caching);
        bool unmapMem(uint64_t vaddr, bool largepages = false);

        bool mapMemRange(uint64_t vaddr, uint64_t paddr, uint64_t size, uint64_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = (flags & LPAGES) ? this->large_page_size : this->page_size;
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->mapMem(vaddr + i, paddr + i, flags, cache)) return false;
            }
            return true;
        }

        bool remapMemRange(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t size, uint64_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = (flags & LPAGES) ? this->large_page_size : this->page_size;
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->remapMem(vaddr_old + i, vaddr_new + i, flags, cache)) return false;
            }
            return true;
        }

        bool unmapMemRange(uint64_t vaddr, uint64_t size, bool largepages = false)
        {
            size_t psize = largepages ? this->large_page_size : this->page_size;
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->unmapMem(vaddr + i, largepages)) return false;
            }
            return true;
        }

        void switchTo();
        void save();

        Pagemap(bool user = false);
    };

    extern Pagemap *kernel_pagemap;
    bool is_canonical(uintptr_t addr);

    void init();
} // namespace mm::vmm