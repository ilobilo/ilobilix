// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/misc.hpp>
#include <lib/lock.hpp>
#include <cstdint>
#include <cstddef>

namespace heap
{
    struct slab_t
    {
        lock_t lock;
        uint64_t firstfree;
        uint64_t size;

        void init(uint64_t size);
        void *alloc();
        void free(void *ptr);
    };

    struct slabHdr
    {
        slab_t *slab;
    };

    class slaballoc
    {
        private:
        lock_t lock;

        struct bigallocMeta
        {
            size_t pages;
            size_t size;
        };

        slab_t *get_slab(size_t size);

        void *big_malloc(size_t size);
        void *big_realloc(void *oldptr, size_t size);
        void big_free(void *ptr);
        size_t big_allocsize(void *ptr);

        public:
        bool initialised = false;
        slab_t slabs[10];

        void init();

        void *malloc(size_t size);
        void *calloc(size_t num, size_t size);
        void *realloc(void *oldptr, size_t size);
        void free(void *ptr);
        size_t allocsize(void *ptr);


        template<typename type = void*>
        type malloc(size_t size)
        {
            return reinterpret_cast<type>(this->malloc(size));
        }

        template<typename type = void*>
        type calloc(size_t num, size_t size)
        {
            return reinterpret_cast<type>(this->calloc(num, size));
        }

        template<typename type = void*>
        type realloc(void *oldptr, size_t size)
        {
            return reinterpret_cast<type>(this->realloc(oldptr, size));
        }

        void free(auto ptr)
        {
            this->free(reinterpret_cast<void*>(ptr));
        }

        size_t allocsize(auto ptr)
        {
            return this->allocsize(reinterpret_cast<void*>(ptr));
        }
    };

    extern slaballoc allocator;
} // namespace heap

template<typename type = void*>
type malloc(size_t size, bool phys = false)
{
    type ret = heap::allocator.malloc<type>(size);
    if (phys) ret = fromhh(ret);
    return ret;
}

template<typename type = void*>
type calloc(size_t num, size_t size, bool phys = false)
{
    type ret = heap::allocator.calloc<type>(num, size);
    if (phys) ret = fromhh(ret);
    return ret;
}

template<typename type = void*>
type realloc(void *oldptr, size_t size, bool phys = false)
{
    type ret = heap::allocator.realloc<type>(oldptr, size);
    if (phys) ret = fromhh(ret);
    return ret;
}

void free(auto ptr, bool phys = false)
{
    heap::allocator.free(phys ? fromhh(ptr) : ptr);
}

size_t allocsize(auto ptr, bool phys = false)
{
    return heap::allocator.allocsize(phys ? fromhh(ptr) : ptr);
}