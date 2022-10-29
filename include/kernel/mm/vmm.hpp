// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <cstdint>

namespace vmm
{
    enum flags
    {
        read = (1 << 0),
        write = (1 << 1),
        exec = (1 << 2),
        user = (1 << 3),
        lpages = (1 << 4),

        rw = read | write,
        rwx = read | write | exec,

        rwu = rw | user,
        rwxu = rwx | user
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
    static constexpr size_t default_flags = rwx;

    struct pagemap
    {
        void *toplvl = nullptr;
        size_t large_page_size = 0;
        size_t page_size = 0;
        lock_t lock;

        uintptr_t virt2phys(uintptr_t vaddr, bool largepages = false);

        bool map(uintptr_t vaddr, uintptr_t paddr, size_t flags = default_flags, caching cache = default_caching);
        bool remap(uintptr_t vaddr_old, uintptr_t vaddr_new, size_t flags = default_flags, caching cache = default_caching);
        bool unmap(uintptr_t vaddr, bool largepages = false);
        bool setflags(uintptr_t vaddr, size_t flags = default_flags, caching cache = default_caching);

        bool map_range(uintptr_t vaddr, uintptr_t paddr, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = (flags & lpages) ? this->large_page_size : this->page_size;
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->map(vaddr + i, paddr + i, flags, cache))
                    return false;
            }
            return true;
        }

        bool remap_range(uintptr_t vaddr_old, uintptr_t vaddr_new, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = (flags & lpages) ? this->large_page_size : this->page_size;
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->remap(vaddr_old + i, vaddr_new + i, flags, cache))\
                    return false;
            }
            return true;
        }

        bool unmap_range(uintptr_t vaddr, size_t size, bool largepages = false)
        {
            size_t psize = largepages ? this->large_page_size : this->page_size;
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->unmap(vaddr + i, largepages))
                    return false;
            }
            return true;
        }

        bool setflags_range(uintptr_t vaddr, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = (flags & lpages) ? this->large_page_size : this->page_size;
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->setflags(vaddr + i, flags, cache))
                    return false;
            }
            return true;
        }

        void load();
        void save();

        pagemap(bool user = false);
    };

    extern pagemap *kernel_pagemap;

    bool is_canonical(uintptr_t addr);

    void init();
    [[gnu::weak]] void arch_init();
} // namespace vmm