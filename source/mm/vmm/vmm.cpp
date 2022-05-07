// Copyright (C) 2022  ilobilo

#include <mm/vmm/vmm.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>

namespace mm::vmm
{
    Pagemap *kernel_pagemap;

    Pagemap *newPagemap(bool user)
    {
        Pagemap *pagemap = new Pagemap();

        if (user == false)
        {
            for (uint64_t i = 0; i < 0x100000000; i += pagemap->large_page_size)
            {
                pagemap->mapMem(i, i, flags::Present | flags::Write, true);
                pagemap->mapMem(tohh(i), i, flags::Present | flags::Write, true);
            }

            for (size_t i = 0; i < memmap_request.response->entry_count; i++)
            {
                limine_memmap_entry *mmap = memmap_request.response->entries[i];

                uint64_t base = align_down(mmap->base, pagemap->page_size);
                uint64_t top = align_up(mmap->base + mmap->length, pagemap->page_size);
                if (top < 0x100000000) continue;

                for (uint64_t t = base; t < top; t += pagemap->page_size)
                {
                    if (t < 0x100000000) continue;
                    pagemap->mapMem(t, t, flags::Present | flags::Write);
                    pagemap->mapMem(t + hhdm_offset, t, flags::Present | flags::Write);
                }
            }
        }

        for (size_t i = 0; i < kernel_file_request.response->kernel_file->size; i += pagemap->page_size)
        {
            uint64_t paddr = kernel_address_request.response->physical_base + i;
            uint64_t vaddr = kernel_address_request.response->virtual_base + i;
            pagemap->mapMem(vaddr, paddr, flags::Present | flags::Write);
        }

        return pagemap;
    }

    void init()
    {
        log::info("Initialising VMM... ");

        kernel_pagemap = newPagemap();
        kernel_pagemap->switchTo();

        log::println("Done!");
    }
} // namespace mm::vmm