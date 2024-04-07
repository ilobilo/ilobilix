// Copyright (C) 2022-2024  ilobilo

#include <init/kernel.hpp>
#include <arch/arch.hpp>

#include <lib/panic.hpp>
#include <lib/math.hpp>
#include <lib/log.hpp>

#include <mm/vmm.hpp>

#include <magic_enum.hpp>

namespace vmm
{
    pagemap *kernel_pagemap = nullptr;
    bool print_errors = true;

    static uintptr_t vspbaddrs[magic_enum::enum_count<vsptypes>() + 1] { };

    void init()
    {
        log::infoln("VMM: Initialising...");

        if (vmm::arch_init)
            vmm::arch_init();

        kernel_pagemap = new pagemap();
        {
            auto [psize, flags] = kernel_pagemap->required_size(gib1 * 4);
            for (uintptr_t i = 0; i < gib1 * 4; i += psize)
            {
                // assert(kernel_pagemap->map(i, i, rw | flags));
                assert(kernel_pagemap->map(tohh(i), i, rw | flags));
            }
        }

        for (size_t i = 0; i < memmap_request.response->entry_count; i++)
        {
            limine_memmap_entry *mmap = memmap_request.response->entries[i];

            uintptr_t base = align_down(mmap->base, kernel_pagemap->get_psize());
            uintptr_t top = align_up(mmap->base + mmap->length, kernel_pagemap->get_psize());
            if (top < gib1 * 4)
                continue;

            caching cache = default_caching;
            if (mmap->type == LIMINE_MEMMAP_FRAMEBUFFER)
                cache = framebuffer;

            auto size = top - base;
            auto [psize, flags] = kernel_pagemap->required_size(size);

            auto alsize = align_down(size, psize);
            auto diff = size - alsize;

            for (uintptr_t t = base; t < (base + alsize); t += psize)
            {
                if (t < gib1 * 4)
                    continue;

                // assert(kernel_pagemap->map(t, t, rw | flags, cache));
                assert(kernel_pagemap->map(tohh(t), t, rw | flags, cache));
            }
            base += alsize;

            for (uintptr_t t = base; t < (base + diff); t += kernel_pagemap->get_psize())
            {
                if (t < gib1 * 4)
                    continue;

                // assert(kernel_pagemap->map(t, t, rw, cache));
                assert(kernel_pagemap->map(tohh(t), t, rw, cache));
            }
        }

        // TODO: Correct perms
        for (size_t i = 0; i < kernel_file_request.response->kernel_file->size; i += kernel_pagemap->get_psize())
        {
            uintptr_t paddr = kernel_address_request.response->physical_base + i;
            uintptr_t vaddr = kernel_address_request.response->virtual_base + i;
            assert(kernel_pagemap->map(vaddr, paddr, rwx, write_back));
        }

        {
            auto base = tohh(align_up(pmm::mem_top, gib1));
            vspbaddrs[0] = base;

            for (size_t i = 1; auto &entry : vspbaddrs)
                entry = base + (gib1 * i);
        }

        kernel_pagemap->load(true);
    }

    uintptr_t alloc_vspace(vsptypes type, size_t increment, size_t alignment)
    {
        auto index = std::to_underlying(type);

        uintptr_t *entry = &vspbaddrs[index];
        if (type != vsptypes::other && increment > 0 && *entry + increment > (vspbaddrs[0] + (gib1 * (index + 1))))
            entry = &vspbaddrs[std::to_underlying(vsptypes::other)];

        uintptr_t ret = alignment ? align_up(*entry, alignment) : *entry;
        *entry += increment + (ret - *entry);

        return ret;
    }

    void *pagemap::get_next_lvl(ptentry &entry, bool allocate, uintptr_t vaddr, size_t opsize, size_t psize)
    {
        void *ret = nullptr;

        if (entry.is_valid())
        {
            if (entry.is_large() && opsize != static_cast<size_t>(-1))
            {
                auto [old_flags, old_caching] = arch2flags(entry.getflags(), opsize > this->page_size);
                auto old_paddr = entry.getaddr();
                auto old_vaddr = vaddr & ~(opsize - 1);

                if (old_paddr & (opsize - 1))
                    PANIC("VMM: Unexpected page table entry address: 0x{:X}", old_paddr);

                ret = vmm::arch::alloc_ptable();
                entry.setaddr(fromhh(reinterpret_cast<uintptr_t>(ret)));
                entry.setflags(arch::new_table_flags, true);

                for (size_t i = 0; i < opsize; i += psize)
                    this->map_nolock(old_vaddr + i, old_paddr + i, old_flags | this->get_psize_flags(psize), old_caching);
            }
            else ret = tohh(reinterpret_cast<void*>(entry.getaddr()));
        }
        else if (allocate == true)
        {
            ret = vmm::arch::alloc_ptable();
            entry.setaddr(fromhh(reinterpret_cast<uintptr_t>(ret)));
            entry.setflags(arch::new_table_flags, true);
        }

        return ret;
    }
} // namespace vmm