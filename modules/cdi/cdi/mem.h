/*
 * Copyright (c) 2010 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_MEM_H_
#define _CDI_MEM_H_

#include <stdint.h>
#include <cdi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CDI_MEM_ALIGN_MASK = 0x1F,
    CDI_MEM_VIRT_ONLY = 0x20,
    CDI_MEM_PHYS_CONTIGUOUS = 0x40,
    CDI_MEM_DMA_16M = 0x80,
    CDI_MEM_DMA_4G = 0x100,
    CDI_MEM_NOINIT = 0x200,
} cdi_mem_flags_t;

struct cdi_mem_sg_item
{
    uintptr_t start;
    size_t size;
};

struct cdi_mem_sg_list
{
    size_t num;
    struct cdi_mem_sg_item* items;
};

struct cdi_mem_area
{
    size_t size;
    void* vaddr;
    struct cdi_mem_sg_list paddr;
    cdi_mem_osdep osdep;
};

struct cdi_mem_area *cdi_mem_alloc(size_t size, cdi_mem_flags_t flags);
struct cdi_mem_area *cdi_mem_map(uintptr_t paddr, size_t size);
void cdi_mem_free(struct cdi_mem_area *p);
struct cdi_mem_area *cdi_mem_require_flags(struct cdi_mem_area *p, cdi_mem_flags_t flags);
int cdi_mem_copy(struct cdi_mem_area *dest, struct cdi_mem_area *src);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

