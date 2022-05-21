// Copyright (C) 2022  ilobilo

#include <cdi/mempool.h>

extern "C"
{
    cdi_mempool *cdi_mempool_create(size_t pool_size, size_t object_size);
    int cdi_mempool_get(cdi_mempool *pool, void **obj, uintptr_t *phys_obj);
    int cdi_mempool_put(cdi_mempool *pool, void *obj);
} // extern "C"