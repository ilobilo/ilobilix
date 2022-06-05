// Copyright (C) 2022  ilobilo

#include <lib/alloc.hpp>
#include <cdi/mem.h>
#include <string.h>

extern "C"
{
    cdi_mem_area *cdi_mem_alloc(size_t size, cdi_mem_flags_t flags)
    {
        auto vaddr = malloc(size);
        auto area = new cdi_mem_area
        {
            .size = size,
            .vaddr = vaddr,
            .paddr = {
                .num = (flags & CDI_MEM_VIRT_ONLY ? size_t(0) : size_t(1)),
                .items = (flags & CDI_MEM_VIRT_ONLY ? nullptr : new cdi_mem_sg_item
                {
                    .start = fromhh(reinterpret_cast<uintptr_t>(vaddr)),
                    .size = size
                })
            },
            .osdep = {
                .malloced = true
            }
        };
        return area;
    }

    cdi_mem_area *cdi_mem_map(uintptr_t paddr, size_t size)
    {
        auto area = new cdi_mem_area
        {
            .size = size,
            .vaddr = reinterpret_cast<void*>(tohh(paddr)), // Really map memory?
            .paddr = {
                .num = 1,
                .items = new cdi_mem_sg_item
                {
                    .start = paddr,
                    .size = size
                }
            },
            .osdep = {
                .malloced = false
            }
        };
        return area;
    }

    void cdi_mem_free(cdi_mem_area *p)
    {
        if (p->osdep.malloced == true) free(p->vaddr);
        delete p->paddr.items;
        delete p;
    }

    cdi_mem_area *cdi_mem_require_flags(cdi_mem_area *p, cdi_mem_flags_t flags)
    {
        auto area = cdi_mem_alloc(p->size, flags);
        memcpy(area->vaddr, p->vaddr, area->size);
        return area;
    }

    int cdi_mem_copy(cdi_mem_area *dest, cdi_mem_area *src)
    {
        if (dest->size != src->size) return -1;
        if (dest->vaddr != src->vaddr) memmove(dest->vaddr, src->vaddr, dest->size);
        return 0;
    }
} // extern "C"