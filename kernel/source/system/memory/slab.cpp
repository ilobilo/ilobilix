// Copyright (C) 2022-2024  ilobilo

module system.memory.slab;

import system.memory.phys;
import frigg;
import lib;
import cppstd;

namespace slab
{
    struct policy
    {
        static inline constexpr std::size_t page_size = pmm::page_size;

        std::uintptr_t map(std::size_t length)
        {
            return lib::tohh(pmm::alloc<std::uintptr_t>(lib::div_roundup(length, pmm::page_size)));
        }

        void unmap(std::uintptr_t addr, std::size_t length)
        {
            pmm::free(lib::fromhh(addr), lib::div_roundup(length, pmm::page_size));
        }
    };

    constinit policy valloc;
    constinit frg::manual_box<frg::slab_pool<policy, lib::spinlock>> pool;
    constinit frg::manual_box<frg::slab_allocator<policy, lib::spinlock>> kalloc;

    void *alloc(std::size_t size)
    {
        return kalloc->allocate(size);
    }

    void *realloc(void *oldptr, std::size_t size)
    {
        return kalloc->reallocate(oldptr, size);
    }

    void free(void *ptr)
    {
        return kalloc->free(ptr);
    }

    void init()
    {
        log::info("heap: initialising the slab allocator");

        pool.initialize(valloc);
        kalloc.initialize(pool.get());
    }
} // namespace slab