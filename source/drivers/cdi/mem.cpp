// Copyright (C) 2022  ilobilo

#include <cdi/mem.h>

extern "C"
{
    cdi_mem_area *cdi_mem_alloc(size_t size, cdi_mem_flags_t flags);
    cdi_mem_area *cdi_mem_map(uintptr_t paddr, size_t size);
    void cdi_mem_free(cdi_mem_area *p);
    cdi_mem_area *cdi_mem_require_flags(cdi_mem_area *p, cdi_mem_flags_t flags);
    int cdi_mem_copy(cdi_mem_area *dest, cdi_mem_area *src);
} // extern "C"