// Copyright (C) 2024  ilobilo

export module system.memory.virt:pagemap;

import magic_enum;
import lib;
import std;

export namespace vmm
{
    enum class caching
    {
        uncacheable,
        uncacheable_strong,
        write_through,
        write_protected,
        write_combining,
        write_back,

        normal = write_back,
        mmio = uncacheable_strong,
        framebuffer = write_combining
    };

    enum class flag
    {
        none = 0,
        read = (1 << 0),
        write = (1 << 1),
        exec = (1 << 2),
        user = (1 << 3),
        global = (1 << 4),

        rw = read | write,
        rwx = read | write | exec,
        rwu = rw | user,
        rwxu = rwx | user
    };

    using magic_enum::bitwise_operators::operator~;
    using magic_enum::bitwise_operators::operator&;
    using magic_enum::bitwise_operators::operator&=;
    using magic_enum::bitwise_operators::operator|;
    using magic_enum::bitwise_operators::operator|=;

    enum class page_size
    {
        // do not modify
        small,
        medium,
        large,

        normal = small
    };

    enum class error
    {
        addr_not_aligned,
        not_mapped,
        already_mapped,
        invalid_entry
    };

    class pagemap
    {
        private:
        static std::uintptr_t pa_mask;

        static const std::uintptr_t valid_table_flags;
        static const std::uintptr_t new_table_flags;

        struct entry
        {
            std::uintptr_t value = 0;

            void clear() { value = 0; }

            void setflags(std::uintptr_t aflags, bool enabled)
            {
                if (enabled)
                    value |= aflags;
                else
                    value &= ~aflags;
            }

            bool getflags(std::uintptr_t aflags) const
            {
                return (value & aflags) == aflags;
            }

            std::uintptr_t getflags() const
            {
                return value & ~pa_mask;
            }

            void setaddr(std::uintptr_t paddr)
            {
                value = (value & ~pa_mask) | (paddr & pa_mask);
            }

            std::uintptr_t getaddr() const
            {
                return value & pa_mask;
            }
        };

        struct table;
        table *_table;
        std::mutex _lock;

        static table *new_table();

        static page_size fixpsize(page_size psize);
        static void invalidate(std::uintptr_t vaddr);

        static std::uintptr_t to_arch(flag flags, caching cache, page_size psize);
        static auto from_arch(std::uintptr_t flags, page_size psize) -> std::pair<flag, caching>;

        auto getpte(std::uintptr_t vaddr, page_size psize, bool allocate) -> std::expected<std::reference_wrapper<entry>, error>;

        static auto getlvl(entry &entry, bool allocate) -> table *;

        public:
        static std::size_t from_page_size(page_size psize);
        static page_size max_page_size(std::size_t size);

        std::expected<void, error> map(std::uintptr_t vaddr, std::uintptr_t paddr, std::size_t length, flag flags = flag::rw, page_size psize = page_size::normal, caching cache = caching::normal);
        std::expected<void, error> unmap(std::uintptr_t vaddr, std::size_t length, page_size psize = page_size::normal);

        std::expected<std::uintptr_t, error> translate(std::uintptr_t vaddr, page_size psize = page_size::normal);

        void store();
        void load() const;

        pagemap();
        ~pagemap();
    };

    inline lib::lazy_init<pagemap> kernel_pagemap;
} // export namespace vmm