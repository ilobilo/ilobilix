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
        global = (1 << 4),
        lpage = (1 << 5),
        llpage = (1 << 6),

        rw = read | write,
        rwx = read | write | exec,

        rwu = rw | user,
        rwxu = rwx | user
    };
    #define islpage(x) ({ x & lpage || x & llpage; })

    enum caching
    {
        uncachable,
        write_through,
        write_protected,
        write_combining,
        write_back,

        mmio = uncachable,
        framebuffer = write_combining
    };

    static constexpr caching default_caching = write_back;
    static constexpr size_t default_flags = rwx;

    static constexpr size_t kib4 = 0x1000;
    static constexpr size_t mib2 = 0x200000;
    static constexpr size_t gib1 = 0x40000000;

    extern uintptr_t pa_mask;

    struct ptentry
    {
        uintptr_t value = 0;

        void setflags(uintptr_t flags, bool enabled)
        {
            auto temp = this->value;
            temp &= ~flags;
            if (enabled)
                temp |= flags;
            this->value = temp;
        }

        bool getflags(uintptr_t flags)
        {
            return (this->value & flags) ? true : false;
        }

        uintptr_t getflags()
        {
            return this->value & ~pa_mask;
        }

        uintptr_t getaddr()
        {
            return this->value & pa_mask;
        }

        void setaddr(uintptr_t address)
        {
            auto temp = this->value;
            temp &= ~pa_mask;
            temp |= address;
            this->value = temp;
        }
    };

    struct ptable;
    struct pagemap
    {
        ptable *toplvl = nullptr;
        size_t llpage_size = 0;
        size_t lpage_size = 0;
        size_t page_size = 0;
        lock_t lock;

        inline size_t get_psize(size_t flags)
        {
            size_t psize = this->page_size;
            if (flags & lpage)
                psize = this->lpage_size;
            if (flags & llpage)
                psize = this->llpage_size;
            return psize;
        }

        inline std::pair<size_t, size_t> required_size(size_t size)
        {
            if (size >= this->llpage_size)
                return { this->llpage_size, llpage };
            else if (size >= this->lpage_size)
                return { this->lpage_size, lpage };

            return { this->page_size, 0 };
        }

        ptentry *virt2pte(uint64_t vaddr, bool allocate, uint64_t psize);

        uintptr_t virt2phys(uintptr_t vaddr, size_t flags = 0);

        bool map(uintptr_t vaddr, uintptr_t paddr, size_t flags = default_flags, caching cache = default_caching);
        bool unmap(uintptr_t vaddr, size_t flags = 0);

        inline bool remap(uintptr_t vaddr_old, uintptr_t vaddr_new, size_t flags = default_flags, caching cache = default_caching)
        {
            uint64_t paddr = this->virt2phys(vaddr_old, flags);
            if (!this->unmap(vaddr_old, flags))
                return false;

            if (!this->map(vaddr_new, paddr, flags, cache))
                return false;

            return true;
        }

        bool setflags(uintptr_t vaddr, size_t flags = default_flags, caching cache = default_caching);

        inline bool map_range(uintptr_t vaddr, uintptr_t paddr, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->map(vaddr + i, paddr + i, flags, cache))
                    return false;
            }
            return true;
        }

        inline bool unmap_range(uintptr_t vaddr, size_t size, size_t flags = 0)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->unmap(vaddr + i, flags))
                    return false;
            }
            return true;
        }

        inline bool remap_range(uintptr_t vaddr_old, uintptr_t vaddr_new, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->remap(vaddr_old + i, vaddr_new + i, flags, cache))\
                    return false;
            }
            return true;
        }

        inline bool setflags_range(uintptr_t vaddr, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->setflags(vaddr + i, flags, cache))
                    return false;
            }
            return true;
        }

        void load();
        void save();

        pagemap();
    };

    extern pagemap *kernel_pagemap;

    bool is_canonical(uintptr_t addr);
    uintptr_t flags2arch(size_t flags);

    void init();
    [[gnu::weak]] void arch_init();
} // namespace vmm