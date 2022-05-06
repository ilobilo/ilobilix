// Copyright (C) 2024  ilobilo

module system.memory.phys;

import boot;
import lib;
import std;

namespace pmm
{
    namespace
    {
        constexpr std::size_t available = 0;
        constexpr std::size_t used = 1;

        constinit std::mutex lock;
        constinit lib::bitmap bitmap;
        std::size_t index = 0;

        memory mem;
    } // namespace

    memory info()
    {
        std::unique_lock _ { lock };
        return mem;
    }

    void *alloc(std::size_t count)
    {
        if (count == 0)
            return nullptr;

        std::unique_lock _ { lock };

        auto inner_alloc = [count](auto limit) -> void*
        {
            std::size_t p = 0;
            while (index < limit)
            {
                if (bitmap[index++] == available)
                {
                    if (++p == count)
                    {
                        auto page = index - count;
                        for (std::size_t i = page; i < index; i++)
                            bitmap[i] = used;
                        return reinterpret_cast<void *>(page * page_size);
                    }
                }
                else p = 0;
            }
            return nullptr;
        };

        auto i = index;
        void *ret = inner_alloc(mem.usable_top / page_size);
        if (ret == nullptr)
        {
            index = 0;
            ret = inner_alloc(i);

            if (ret == nullptr)
                lib::panic("Out of physical memory to allocate");
        }
        std::memset(lib::tohh(ret), 0, count * page_size);

        mem.used += count * page_size;
        return ret;
    }

    void free(void *ptr, std::size_t count)
    {
        if (ptr == nullptr)
            return;

        std::unique_lock _ { lock };

        std::size_t page = reinterpret_cast<std::uintptr_t>(ptr) / page_size;

        for (std::size_t i = page; i < page + count; i++)
            bitmap[i] = available;

        mem.used -= count * page_size;
    }

    void reclaim()
    {
        log::debug("Reclaiming bootloader memory");

        auto memmaps = boot::requests::memmap.response->entries;
        std::size_t num = boot::requests::memmap.response->entry_count;

        for (std::size_t i = 0; i < num; i++)
        {
            auto memmap = memmaps[i];
            if (static_cast<boot::memmap>(memmap->type) != boot::memmap::bootloader_reclaimable)
                continue;

            free(reinterpret_cast<void *>(memmap->base), memmap->length / page_size);
        }
    }

    void init()
    {
        log::info("Initialising the physical memory manager");

        auto memmaps = boot::requests::memmap.response->entries;
        std::size_t num = boot::requests::memmap.response->entry_count;

        log::debug("Memory map entries: {}", num);

        for (std::size_t i = 0; i < num; i++)
        {
            auto memmap = memmaps[i];

            std::uintptr_t end = memmap->base + memmap->length;
            mem.top = std::max(end, mem.top);

            switch (static_cast<boot::memmap>(memmap->type))
            {
                case boot::memmap::kernel_and_modules:
                case boot::memmap::bootloader_reclaimable:
                    mem.used += memmap->length;
                    [[fallthrough]];
                case boot::memmap::usable:
                    mem.usable += memmap->length;
                    mem.usable_top = std::max(mem.usable_top, end);
                    break;
                default:
                    continue;
            }

            mem.total += memmaps[i]->length;
        }

        std::size_t bitmap_entries = mem.usable_top / page_size;
        const std::size_t bitmap_size = lib::align_up(bitmap_entries / 8, page_size);
        bitmap_entries = bitmap_size * 8;

        bool found = false;
        for (std::size_t i = 0; i < num; i++)
        {
            auto memmap = memmaps[i];
            if (static_cast<boot::memmap>(memmap->type) != boot::memmap::usable)
                continue;

            if (memmap->length >= bitmap_size)
            {
                const auto addr = lib::tohh(memmap->base);

                auto data = reinterpret_cast<std::uint8_t *>(addr);
                std::memset(data, 0xFF, bitmap_entries);
                bitmap.initialise(data, bitmap_entries);

                log::debug("Bitmap address: 0x{:X}, size: {} Bytes", addr, bitmap_size);

                memmap->length -= bitmap_size;
                memmap->base += bitmap_size;

                mem.used += bitmap_size;

                found = true;
                break;
            }
        }

        if (found == false)
            lib::panic("Could not find large enough continuous usable memory space for the pmm bitmap");

        for (std::size_t i = 0; i < num; i++)
        {
            const auto memmap = memmaps[i];
            if (static_cast<boot::memmap>(memmap->type) != boot::memmap::usable)
                continue;

            for (std::uintptr_t ii = 0; ii < memmap->length; ii += page_size)
                bitmap[(memmap->base + ii) / page_size] = available;
        }

        log::info("Usable memory: {} Mebibytes", mem.usable / 1024 / 1024);
    }
} // namespace pmm