// Copyright (C) 2022  ilobilo

#include <cstddef>
#include <cstdint>

namespace mm::pmm
{
    static constexpr size_t block_size = 0x1000;
    extern uint64_t mem_top;

    size_t freemem();
    size_t usedmem();

    void *alloc(size_t count = 1);

    template<typename type = void*>
    type alloc(size_t count = 1)
    {
        return reinterpret_cast<type>(alloc(count));
    }

    void free(void *ptr, size_t count = 1);

    void init();
} // namespace mm::pmm