// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/types.hpp>
#include <lib/lock.hpp>
#include <cstdint>
#include <vector>

namespace vfs { struct resource; }
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

    static constexpr auto invalid_addr = uintptr_t(-1);

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

    namespace mmap
    {
        struct global;
        struct local;

        #define MAP_FAILED ((void*)-1)

        #define MAP_SHARED 0x01
        #define MAP_PRIVATE 0x02
        #define MAP_SHARED_VALIDATE 0x03
        #define MAP_TYPE 0x0f
        #define MAP_FIXED 0x10
        #define MAP_ANON 0x20
        #define MAP_ANONYMOUS MAP_ANON
        #define MAP_NORESERVE 0x4000
        #define MAP_GROWSDOWN 0x0100
        #define MAP_DENYWRITE 0x0800
        #define MAP_EXECUTABLE 0x1000
        #define MAP_LOCKED 0x2000
        #define MAP_POPULATE 0x8000
        #define MAP_NONBLOCK 0x10000
        #define MAP_STACK 0x20000
        #define MAP_HUGETLB 0x40000
        #define MAP_SYNC 0x80000
        #define MAP_FIXED_NOREPLACE 0x100000
        #define MAP_FILE 0

        #define PROT_NONE 0
        #define PROT_READ 1
        #define PROT_WRITE 2
        #define PROT_EXEC 4
        #define PROT_GROWSDOWN 0x01000000
        #define PROT_GROWSUP 0x02000000
    } // namespace mmap

    struct ptable;
    struct pagemap
    {
        std::vector<std::shared_ptr<mmap::local>> ranges;
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

        std::optional<std::tuple<std::shared_ptr<mmap::local>, size_t, size_t>> addr2range(uintptr_t addr);

        ptentry *virt2pte(uint64_t vaddr, bool allocate, uint64_t psize);
        uintptr_t virt2phys(uintptr_t vaddr, size_t flags = 0);

        bool map(uintptr_t vaddr, uintptr_t paddr, size_t flags = default_flags, caching cache = default_caching);
        bool unmap_nolock(uintptr_t vaddr, size_t flags = 0);
        bool setflags(uintptr_t vaddr, size_t flags = default_flags, caching cache = default_caching);

        inline bool unmap(uintptr_t vaddr, size_t flags = 0)
        {
            lockit(this->lock);
            return this->unmap_nolock(vaddr, flags);
        }

        inline bool remap(uintptr_t vaddr_old, uintptr_t vaddr_new, size_t flags = default_flags, caching cache = default_caching)
        {
            uint64_t paddr = this->virt2phys(vaddr_old, flags);
            this->unmap(vaddr_old, flags);
            return this->map(vaddr_new, paddr, flags, cache);
        }

        inline void map_range(uintptr_t vaddr, uintptr_t paddr, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
                this->map(vaddr + i, paddr + i, flags, cache);
        }

        inline void unmap_range(uintptr_t vaddr, size_t size, size_t flags = 0)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
                this->unmap(vaddr + i, flags);
        }

        inline void remap_range(uintptr_t vaddr_old, uintptr_t vaddr_new, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
                this->remap(vaddr_old + i, vaddr_new + i, flags, cache);
        }

        inline void setflags_range(uintptr_t vaddr, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
                this->setflags(vaddr + i, flags, cache);
        }

        bool mmap_range(uintptr_t vaddr, uintptr_t paddr, size_t length, int prot, int flags);

        void *mmap(uintptr_t addr, size_t length, int prot, int flags, vfs::resource *res, off_t offset);
        bool munmap(uintptr_t addr, size_t length);

        pagemap *fork();

        void load();
        void save();

        pagemap();
        ~pagemap();
    };

    extern pagemap *kernel_pagemap;

    bool is_canonical(uintptr_t addr);
    uintptr_t flags2arch(size_t flags);

    bool page_fault(uintptr_t addr);

    void init();

    void arch_destroy_pmap(pagemap *pmap);
    [[gnu::weak]] void arch_init();
} // namespace vmm