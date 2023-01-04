// Copyright (C) 2022  ilobilo

#include <init/kernel.hpp>
#include <lib/bitmap.hpp>
#include <lib/panic.hpp>
#include <lib/lock.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/pmm.hpp>
#include <algorithm>
#include <cstring>

namespace pmm
{
    static uintptr_t mem_usable_top = 0;
    static size_t lastindex = 0;
    static bitmap_t bitmap;
    static lock_t lock;

    size_t usablemem = 0;
    size_t totalmem = 0;
    size_t usedmem = 0;

    uintptr_t mem_top = 0;

    size_t total()
    {
        return totalmem;
    }

    size_t usable()
    {
        return usablemem;
    }

    size_t used()
    {
        return usedmem;
    }

    size_t free()
    {
        return usablemem - usedmem;
    }

    void *alloc(size_t count)
    {
        if (count == 0)
            return nullptr;

        lockit(lock);
        auto inner_alloc = [count](size_t limit) -> void*
        {
            size_t p = 0;
            while (lastindex < limit)
            {
                if (bitmap[lastindex++] == false)
                {
                    if (++p == count)
                    {
                        size_t page = lastindex - count;
                        for (size_t i = page; i < lastindex; i++)
                            bitmap[i] = true;
                        return reinterpret_cast<void*>(page * page_size);
                    }
                }
                else p = 0;
            }
            return nullptr;
        };

        size_t i = lastindex;
        void *ret = inner_alloc(mem_usable_top / page_size);
        if (ret == nullptr)
        {
            lastindex = 0;
            ret = inner_alloc(i);
            if (ret == nullptr)
                PANIC("PMM: Out of memory!");
        }
        memset(tohh(ret), 0, count * page_size);

        usedmem += count * page_size;
        return ret;
    }

    void free(void *ptr, size_t count)
    {
        if (ptr == nullptr)
            return;

        lockit(lock);
        size_t page = reinterpret_cast<size_t>(ptr) / page_size;
        for (size_t i = page; i < page + count; i++)
            bitmap[i] = false;

        // TODO: Should I remove this?
        lastindex = std::min(lastindex, page);
        usedmem -= count * page_size;
    }

    void init()
    {
        log::infoln("PMM: Initialising...");

        limine_memmap_entry **memmaps = memmap_request.response->entries;
        size_t memmap_count = memmap_request.response->entry_count;

        for (size_t i = 0; i < memmap_count; i++)
        {
            uintptr_t top = memmaps[i]->base + memmaps[i]->length;

            mem_top = std::max(mem_top, top);

            switch (memmaps[i]->type)
            {
                case LIMINE_MEMMAP_USABLE:
                    usablemem += memmaps[i]->length;
                    mem_usable_top = std::max(mem_usable_top, top);
                    [[fallthrough]];
                case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                    totalmem += memmaps[i]->length;
                    break;
            }
        }

        size_t bitmapSize = align_up((mem_usable_top / page_size) / 8, page_size);

        for (size_t i = 0; i < memmap_count; i++)
        {
            if (memmaps[i]->type != LIMINE_MEMMAP_USABLE)
                continue;

            if (memmaps[i]->length >= bitmapSize)
            {
                bitmap.buffer = reinterpret_cast<uint8_t*>(tohh(memmaps[i]->base));
                bitmap.size = bitmapSize;
                memset(bitmap.buffer, 0xFF, bitmap.size);

                memmaps[i]->length -= bitmapSize;
                memmaps[i]->base += bitmapSize;

                usedmem += bitmapSize;
                break;
            }
        }

        for (size_t i = 0; i < memmap_count; i++)
        {
            if (memmaps[i]->type != LIMINE_MEMMAP_USABLE)
                continue;

            for (uintptr_t t = 0; t < memmaps[i]->length; t += page_size)
                bitmap[(memmaps[i]->base + t) / page_size] = false;
        }

        mem_top = align_up(mem_top, 0x40000000);
        heap::allocator.initialize();
    }
} // namespace pmm