// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <lib/types.hpp>
#include <optional>
#include <cstdint>
#include <vector>
#include <mutex>

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
        uncacheable,
        uncacheable_strong,
        write_through,
        write_protected,
        write_combining,
        write_back,

        mmio = uncacheable_strong,
        framebuffer = write_combining
    };

    inline constexpr caching default_caching = write_back;
    inline constexpr size_t default_flags = rw;

    inline constexpr size_t kib4 = 0x1000;
    inline constexpr size_t mib2 = 0x200000;
    inline constexpr size_t gib1 = 0x40000000;

    inline constexpr auto invalid_addr = uintptr_t(-1);

    namespace arch
    {
        extern uintptr_t pa_mask;
        extern uintptr_t new_table_flags;

        extern void *alloc_ptable();
    } // namespace arch

    struct ptentry
    {
        uintptr_t value = 0;

        void reset()
        {
            this->value = 0;
        }

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
            return this->value & ~arch::pa_mask;
        }

        uintptr_t getaddr()
        {
            return this->value & arch::pa_mask;
        }

        void setaddr(uintptr_t address)
        {
            // address &= arch::pa_mask;

            auto temp = this->value;
            temp &= ~arch::pa_mask;
            temp |= address;
            this->value = temp;
        }

        bool is_valid();
        bool is_large();
    };

    namespace mmap
    {
        struct global;
        struct local;

        inline constexpr auto map_failed = fold(reinterpret_cast<void*>(-1));

        inline constexpr auto map_shared = 0x01;
        inline constexpr auto map_private = 0x02;
        inline constexpr auto map_shared_validate = 0x03;
        inline constexpr auto map_type = 0x0F;
        inline constexpr auto map_fixed = 0x10;
        inline constexpr auto map_anon = 0x20;
        inline constexpr auto map_anonymous = map_anon;
        inline constexpr auto map_noreserve = 0x4000;
        inline constexpr auto map_growsdown = 0x0100;
        inline constexpr auto map_denywrite = 0x0800;
        inline constexpr auto map_executable = 0x1000;
        inline constexpr auto map_locked = 0x2000;
        inline constexpr auto map_populate = 0x8000;
        inline constexpr auto map_nonblock = 0x10000;
        inline constexpr auto map_stack = 0x20000;
        inline constexpr auto map_hugetlb = 0x40000;
        inline constexpr auto map_sync = 0x80000;
        inline constexpr auto map_fixed_noreplace = 0x100000;
        inline constexpr auto map_file = 0;

        inline constexpr auto prot_none = 0;
        inline constexpr auto prot_read = 1;
        inline constexpr auto prot_write = 2;
        inline constexpr auto prot_exec = 4;
        inline constexpr auto prot_growsdown = 0x01000000;
        inline constexpr auto prot_growsup = 0x02000000;

        inline constexpr uintptr_t def_bump_base = 0x80000000000;
    } // namespace mmap

    struct ptable;
    struct pagemap
    {
        private:
        std::vector<std::shared_ptr<mmap::local>> ranges;
        uintptr_t mmap_bump_base = mmap::def_bump_base;

        ptable *toplvl = nullptr;

        size_t llpage_size = 0;
        size_t lpage_size = 0;
        size_t page_size = 0;
        std::mutex lock;

        public:
        auto get_raw() const { return this->toplvl; }

        size_t get_psize(size_t flags = 0) const
        {
            size_t psize = this->page_size;
            if (flags & lpage)
                psize = this->lpage_size;
            if (flags & llpage)
                psize = this->llpage_size;
            return psize;
        }

        size_t get_psize_flags(size_t psize) const
        {
            if (psize == this->lpage_size)
                return lpage;
            if (psize == this->llpage_size)
                return llpage;
            return 0;
        }

        std::pair<size_t, size_t> required_size(size_t size) const
        {
            if (size >= this->llpage_size)
                return { this->llpage_size, llpage };
            else if (size >= this->lpage_size)
                return { this->lpage_size, lpage };

            return { this->page_size, 0 };
        }

        std::optional<std::tuple<std::shared_ptr<mmap::local>, size_t, size_t>> addr2range(uintptr_t addr);
        void *get_next_lvl(ptentry &entry, bool allocate, uintptr_t vaddr = -1, size_t opsize = -1, size_t psize = -1);

        ptentry *virt2pte(uint64_t vaddr, bool allocate, uint64_t psize, bool checkll);
        uintptr_t virt2phys(uintptr_t vaddr, size_t flags = 0);

        bool map_nolock(uintptr_t vaddr, uintptr_t paddr, size_t flags = default_flags, caching cache = default_caching);
        bool unmap_nolock(uintptr_t vaddr, size_t flags = 0);
        bool setflags_nolock(uintptr_t vaddr, size_t flags = default_flags, caching cache = default_caching);

        bool map(uintptr_t vaddr, uintptr_t paddr, size_t flags = default_flags, caching cache = default_caching)
        {
            std::unique_lock guard(this->lock);
            return this->map_nolock(vaddr, paddr, flags, cache);
        }

        bool unmap(uintptr_t vaddr, size_t flags = 0)
        {
            std::unique_lock guard(this->lock);
            return this->unmap_nolock(vaddr, flags);
        }

        bool setflags(uintptr_t vaddr, size_t flags = default_flags, caching cache = default_caching)
        {
            std::unique_lock guard(this->lock);
            return this->setflags_nolock(vaddr, flags, cache);
        }

        bool remap(uintptr_t vaddr_old, uintptr_t vaddr_new, size_t flags = default_flags, caching cache = default_caching)
        {
            uint64_t paddr = this->virt2phys(vaddr_old, flags);
            this->unmap(vaddr_old, flags);
            return this->map(vaddr_new, paddr, flags, cache);
        }

        bool map_range(uintptr_t vaddr, uintptr_t paddr, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
            {
                if (!this->map(vaddr + i, paddr + i, flags, cache))
                {
                    this->unmap_range(vaddr, i - psize);
                    return false;
                }
            }
            return true;
        }

        bool unmap_range(uintptr_t vaddr, size_t size, size_t flags = 0)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
                if (!this->unmap(vaddr + i, flags))
                    return false;

            return true;
        }

        bool remap_range(uintptr_t vaddr_old, uintptr_t vaddr_new, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
                if (!this->remap(vaddr_old + i, vaddr_new + i, flags, cache))
                    return false;

            return true;
        }

        bool setflags_range(uintptr_t vaddr, size_t size, size_t flags = default_flags, caching cache = default_caching)
        {
            size_t psize = this->get_psize(flags);
            for (size_t i = 0; i < size; i += psize)
                if (!this->setflags(vaddr + i, flags, cache))
                    return false;

            return true;
        }

        bool mmap_range(uintptr_t vaddr, uintptr_t paddr, size_t length, int prot, int flags);

        void *mmap(uintptr_t addr, size_t length, int prot, int flags, vfs::resource *res, off_t offset);
        bool mprotect(uintptr_t addr, size_t length, int prot);
        bool munmap(uintptr_t addr, size_t length);

        void load(bool hh);
        void save();

        pagemap();
        pagemap(pagemap *other);

        ~pagemap();
    };

    extern pagemap *kernel_pagemap;
    extern bool print_errors;

    bool is_canonical(uintptr_t addr);
    uintptr_t flags2arch(size_t flags);
    std::pair<size_t, caching> arch2flags(uintptr_t flags, bool lpages);

    bool page_fault(uintptr_t addr);

    void init();

    void arch_destroy_pmap(pagemap *pmap);
    [[gnu::weak]] void arch_init();

    enum class vsptypes
    {
        modules,
        uacpi,
        ecam, bars,
        other
    };
    uintptr_t alloc_vspace(vsptypes type, size_t increment = 0, size_t alignment = 0);
} // namespace vmm