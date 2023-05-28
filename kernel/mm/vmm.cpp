// Copyright (C) 2022-2023  ilobilo

#include <arch/arch.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <lai/host.h>
#include <mm/vmm.hpp>

namespace vmm
{
    pagemap *kernel_pagemap = nullptr;
    bool print_errors = true;

    void init()
    {
        log::infoln("VMM: Initialising...");

        if (vmm::arch_init)
            vmm::arch_init();

        kernel_pagemap = new pagemap();

        {
            auto [psize, flags] = kernel_pagemap->required_size(gib1 * 4);
            for (size_t i = 0; i < gib1 * 4; i += psize)
            {
                assert(kernel_pagemap->map(i, i, rwx | flags));
                assert(kernel_pagemap->map(tohh(i), i, rw | flags));
            }
        }

        for (size_t i = 0; i < memmap_request.response->entry_count; i++)
        {
            limine_memmap_entry *mmap = memmap_request.response->entries[i];

            uint64_t base = align_down(mmap->base, kernel_pagemap->page_size);
            uint64_t top = align_up(mmap->base + mmap->length, kernel_pagemap->page_size);
            if (top < gib1 * 4)
                continue;

            caching cache = default_caching;
            if (mmap->type == LIMINE_MEMMAP_FRAMEBUFFER)
                cache = framebuffer;

            auto size = top - base;
            auto [psize, flags] = kernel_pagemap->required_size(size);

            auto alsize = align_down(size, psize);
            auto diff = size - alsize;

            for (uint64_t t = base; t < (base + alsize); t += psize)
            {
                if (t < gib1 * 4)
                    continue;

                assert(kernel_pagemap->map(t, t, rwx | flags, cache));
                assert(kernel_pagemap->map(tohh(t), t, rw | flags, cache));
            }
            base += alsize;

            for (uint64_t t = base; t < (base + diff); t += kernel_pagemap->page_size)
            {
                if (t < gib1 * 4)
                    continue;

                assert(kernel_pagemap->map(t, t, rwx, cache));
                assert(kernel_pagemap->map(tohh(t), t, rw, cache));
            }
        }

        // TODO: Correct perms
        for (size_t i = 0; i < kernel_file_request.response->kernel_file->size; i += kernel_pagemap->page_size)
        {
            uint64_t paddr = kernel_address_request.response->physical_base + i;
            uint64_t vaddr = kernel_address_request.response->virtual_base + i;
            assert(kernel_pagemap->map(vaddr, paddr, rwx));
        }

        kernel_pagemap->load();
    }
} // namespace vmm

void *laihost_map(size_t address, size_t count)
{
    return reinterpret_cast<void*>(tohh(address));
}

void laihost_unmap(void *address, size_t count) { }