// Copyright (C) 2022-2024  ilobilo

#include <lib/alloc.hpp>
#include <lib/math.hpp>
#include <lai/host.h>
#include <mm/pmm.hpp>
#include <cstring>
#include <cstdlib>
#include <cstddef>

namespace heap
{
    frg::manual_box<slaballoc> allocator;

    void slab_t::init(size_t size)
    {
        this->size = size;
        this->firstfree = tohh(pmm::alloc<uintptr_t>());

        auto available = 0x1000 - align_up(sizeof(slabHdr), this->size);
        auto slabptr = reinterpret_cast<slabHdr*>(this->firstfree);
        slabptr->slab = this;
        this->firstfree += align_up(sizeof(slabHdr), this->size);

        auto array = reinterpret_cast<uint64_t*>(this->firstfree);
        auto max = available / this->size - 1;
        auto fact = this->size / 8;
        for (size_t i = 0; i < max; i++)
            array[i * fact] = reinterpret_cast<uint64_t>(&array[(i + 1) * fact]);
        array[max * fact] = 0;
    }

    void *slab_t::alloc()
    {
        std::unique_lock guard(this->lock);
        if (this->firstfree == 0)
            this->init(this->size);

        auto oldfree = reinterpret_cast<uint64_t*>(this->firstfree);
        this->firstfree = oldfree[0];
        memset(oldfree, 0, this->size);
        return oldfree;
    }

    void slab_t::free(void *ptr)
    {
        if (ptr == nullptr)
            return;
        std::unique_lock guard(this->lock);

        auto newhead = static_cast<uint64_t*>(ptr);
        newhead[0] = this->firstfree;
        this->firstfree = reinterpret_cast<uintptr_t>(newhead);
    }

    slaballoc::slaballoc()
    {
        // this->slabs[0].init(8);
        // this->slabs[1].init(16);
        // this->slabs[2].init(24);
        // this->slabs[3].init(32);
        // this->slabs[4].init(48);
        // this->slabs[5].init(64);
        // this->slabs[6].init(128);
        // this->slabs[7].init(256);
        // this->slabs[8].init(512);
        // this->slabs[9].init(1024);

        // theoretically this should use less memory
        this->slabs[0].init(16);
        this->slabs[1].init(32);
        this->slabs[2].init(48);
        this->slabs[3].init(80);
        this->slabs[4].init(128);
        this->slabs[5].init(192);
        this->slabs[6].init(288);
        this->slabs[7].init(448);
        this->slabs[8].init(672);
        this->slabs[9].init(1024);
    }

    slab_t *slaballoc::get_slab(size_t size)
    {
        for (slab_t &slab : this->slabs)
            if (slab.size >= size)
                return &slab;

        return nullptr;
    }

    void *slaballoc::big_malloc(size_t size)
    {
        size_t pages = div_roundup(size, 0x1000);
        void *ptr = tohh(pmm::alloc(pages + 1));

        auto metadata = reinterpret_cast<bigallocMeta*>(ptr);
        metadata->pages = pages;
        metadata->size = size;
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + 0x1000);
    }

    void *slaballoc::big_realloc(void *oldptr, size_t size)
    {
        if (oldptr == nullptr)
            return this->malloc(size);

        bigallocMeta *metadata = reinterpret_cast<bigallocMeta*>(reinterpret_cast<uintptr_t>(oldptr) - 0x1000);
        size_t oldsize = metadata->size;

        if (div_roundup(oldsize, 0x1000) == div_roundup(size, 0x1000))
        {
            metadata->size = size;
            return oldptr;
        }

        if (size == 0)
        {
            this->free(oldptr);
            return nullptr;
        }

        if (size < oldsize)
            oldsize = size;

        void *newptr = this->malloc(size);
        if (newptr == nullptr)
            return oldptr;

        memcpy(newptr, oldptr, oldsize);
        this->free(oldptr);
        return newptr;
    }

    void slaballoc::big_free(void *ptr)
    {
        auto metadata = reinterpret_cast<bigallocMeta*>(reinterpret_cast<uintptr_t>(ptr) - 0x1000);
        pmm::free(fromhh(metadata), metadata->pages + 1);
    }

    size_t slaballoc::big_allocsize(void *ptr)
    {
        return reinterpret_cast<bigallocMeta*>(reinterpret_cast<uintptr_t>(ptr) - 0x1000)->size;
    }

    void *slaballoc::malloc(size_t size)
    {
        slab_t *slab = this->get_slab(size);
        if (slab == nullptr)
            return this->big_malloc(size);
        return slab->alloc();
    }

    void *slaballoc::calloc(size_t num, size_t size)
    {
        void *ptr = this->malloc(num * size);
        if (ptr == nullptr)
            return nullptr;

        memset(ptr, 0, num * size);
        return ptr;
    }

    void *slaballoc::realloc(void *oldptr, size_t size)
    {
        if (oldptr == nullptr)
            return this->malloc(size);

        if ((reinterpret_cast<uintptr_t>(oldptr) & 0xFFF) == 0)
            return this->big_realloc(oldptr, size);

        slab_t *slab = reinterpret_cast<slabHdr*>(reinterpret_cast<uintptr_t>(oldptr) & ~0xFFF)->slab;
        size_t oldsize = slab->size;

        if (size == 0)
        {
            this->free(oldptr);
            return nullptr;
        }
        if (size < oldsize) oldsize = size;

        void *newptr = this->malloc(size);
        if (newptr == nullptr)
            return oldptr;

        memcpy(newptr, oldptr, oldsize);
        this->free(oldptr);
        return newptr;
    }

    void slaballoc::free(void *ptr)
    {
        if (ptr == nullptr)
            return;

        if ((reinterpret_cast<uintptr_t>(ptr) & 0xFFF) == 0)
            return this->big_free(ptr);
        reinterpret_cast<slabHdr*>(reinterpret_cast<uintptr_t>(ptr) & ~0xFFF)->slab->free(ptr);
    }

    size_t slaballoc::allocsize(void *ptr)
    {
        if (ptr == nullptr)
            return 0;

        if ((reinterpret_cast<uintptr_t>(ptr) & 0xFFF) == 0)
            return this->big_allocsize(ptr);
        return reinterpret_cast<slabHdr*>(reinterpret_cast<uintptr_t>(ptr) & ~0xFFF)->slab->size;
    }
} // namespace heap

void *operator new(size_t size)
{
    return malloc(size);
}

void *operator new(size_t size, std::align_val_t)
{
    return malloc(size);
}

void *operator new[](size_t size)
{
    return malloc(size);
}

void *operator new[](size_t size, std::align_val_t)
{
    return malloc(size);
}

void operator delete(void *ptr) noexcept
{
    free(ptr);
}

void operator delete(void *ptr, std::align_val_t) noexcept
{
    free(ptr);
}

void operator delete(void *ptr, size_t) noexcept
{
    free(ptr);
}

void operator delete(void *ptr, size_t, std::align_val_t) noexcept
{
    free(ptr);
}

void operator delete[](void *ptr) noexcept
{
    free(ptr);
}

void operator delete[](void *ptr, std::align_val_t) noexcept
{
    free(ptr);
}

void operator delete[](void *ptr, size_t) noexcept
{
    free(ptr);
}

void operator delete[](void *ptr, size_t, std::align_val_t) noexcept
{
    free(ptr);
}

void *laihost_malloc(size_t size)
{
    return malloc(size);
}

void *laihost_realloc(void *ptr, size_t size, size_t)
{
    return realloc(ptr, size);
}

void laihost_free(void *ptr, size_t)
{
    free(ptr);
}