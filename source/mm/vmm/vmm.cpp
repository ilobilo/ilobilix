// Copyright (C) 2022  ilobilo

#include <mm/vmm/vmm.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <lai/host.h>

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
                    pagemap->mapMem(tohh(t), t, flags::Present | flags::Write);
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
        log::info("Initialising VMM...");

        kernel_pagemap = newPagemap();
        kernel_pagemap->switchTo();
    }
} // namespace mm::vmm

void *laihost_map(size_t address, size_t count)
{
    for (size_t i = 0; i < count; i += 0x1000)
    {
        mm::vmm::kernel_pagemap->mapMem(address + hhdm_offset, address);
    }
    return reinterpret_cast<void*>(address + hhdm_offset);
}

void laihost_unmap(void *address, size_t count)
{
    for (size_t i = 0; i < count; i += 0x1000)
    {
        mm::vmm::kernel_pagemap->unmapMem(reinterpret_cast<uint64_t>(address) + i);
    }
}