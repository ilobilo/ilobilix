// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <cstddef>
#include <cstdint>

namespace pmm
{
    static constexpr size_t page_size = 0x1000;
    extern uintptr_t mem_top;

    size_t total();
    size_t usable();
    size_t used();
    size_t free();

    void *alloc(size_t count = 1);

    template<typename Type = void*>
    inline Type alloc(size_t count = 1)
    {
        return reinterpret_cast<Type>(alloc(count));
    }

    void free(void *ptr, size_t count = 1);

    inline void free(auto ptr, size_t count = 1)
    {
        return free(reinterpret_cast<void*>(ptr), count);
    }

    void init();
} // namespace pmm