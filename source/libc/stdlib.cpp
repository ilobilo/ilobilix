// Copyright (C) 2022  ilobilo

#include <lib/alloc.hpp>

extern "C"
{
    void *malloc(size_t size)
    {
        return malloc<void*>(size, false);
    }

    void *calloc(size_t num, size_t size)
    {
        return calloc<void*>(num, size, false);
    }

    void *realloc(void *oldptr, size_t size)
    {
        return realloc<void*>(oldptr, size, false);
    }

    void free(void *ptr)
    {
        return free(ptr, false);
    }
} // extern "C"