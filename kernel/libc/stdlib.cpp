// Copyright (C) 2022  ilobilo

#include <lib/alloc.hpp>
#include <lib/misc.hpp>

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

    int atoi(const char *str)
    {
        return str2int<long>(str, nullptr, 10);
    }

    long atol(const char *str)
    {
        return str2int<long>(str, nullptr, 10);
    }

    long long atoll(const char *str)
    {
        return str2int<long long>(str, nullptr, 10);
    }

    long strtol(const char *str, char **str_end, int base)
    {
        return str2int<long>(str, str_end, base);
    }

    long long strtoll(const char *str, char **str_end, int base)
    {
        return str2int<long long>(str, str_end, base);
    }

    unsigned long strtoul(const char *str, char **str_end, int base)
    {
        return str2int<unsigned long>(str, str_end, base);
    }

    unsigned long long strtoull(const char *str, char **str_end, int base)
    {
        return str2int<unsigned long long>(str, str_end, base);
    }
} // extern "C"