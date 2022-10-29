// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <lib/misc.hpp>
#include <memory>

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


        template<typename Type = void*>
        Type malloc(size_t size)
        {
            return reinterpret_cast<Type>(this->malloc(size));
        }

        template<typename Type = void*>
        Type calloc(size_t num, size_t size)
        {
            return reinterpret_cast<Type>(this->calloc(num, size));
        }

        template<typename Type>
        Type realloc(Type oldptr, size_t size)
        {
            return reinterpret_cast<Type>(this->realloc(reinterpret_cast<void*>(oldptr), size));
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

template<typename Type = void*>
Type malloc(size_t size, bool phys = false)
{
    Type ret = heap::allocator.malloc<Type>(size);
    if (phys == true)
        ret = fromhh(ret);
    return ret;
}

template<typename Type = void*>
Type calloc(size_t num, size_t size, bool phys = false)
{
    Type ret = heap::allocator.calloc<Type>(num, size);
    if (phys == true)
        ret = fromhh(ret);
    return ret;
}

auto realloc(auto oldptr, size_t size, bool phys = false)
{
    auto ret = heap::allocator.realloc(oldptr, size);
    if (phys == true)
        ret = fromhh(ret);
    return ret;
}

void free(auto ptr, bool phys)
{
    heap::allocator.free(phys ? fromhh(ptr) : ptr);
}

size_t allocsize(auto ptr, bool phys)
{
    return heap::allocator.allocsize(phys ? fromhh(ptr) : ptr);
}

void free(auto ptr)
{
    free(ptr, not ishh(ptr));
}

size_t allocsize(auto ptr)
{
    return allocsize(ptr, not ishh(ptr));
}