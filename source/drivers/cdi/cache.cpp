// Copyright (C) 2022  ilobilo

#include <cdi/cache.h>

extern "C"
{
    cdi_cache* cdi_cache_create(size_t block_size, size_t blkpriv_len, cdi_cache_read_block_t *read_block, cdi_cache_write_block_t *write_block, void *prv_data);
    void cdi_cache_destroy(cdi_cache *cache);
    int cdi_cache_sync(cdi_cache *cache);

    cdi_cache_block* cdi_cache_block_get(cdi_cache *cache, uint64_t blocknum, int noread);
    void cdi_cache_block_release(cdi_cache *cache, cdi_cache_block *block);
    void cdi_cache_block_dirty(cdi_cache *cache, cdi_cache_block *block);
} // extern "C"