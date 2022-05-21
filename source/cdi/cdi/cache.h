/*
 * Copyright (c) 2008 Antoine Kaufmann
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_CACHE_H_
#define _CDI_CACHE_H_

#include <stdint.h>
#include <stddef.h>

struct cdi_cache
{
    size_t block_size;
};

struct cdi_cache_block
{
    uint64_t number;
    void *data;

#ifdef __cplusplus
    void *_private;
#else
    void *private;
#endif
};

typedef int (cdi_cache_read_block_t)(struct cdi_cache *cache, uint64_t block, size_t count, void *dest, void *prv);
typedef int (cdi_cache_write_block_t)(struct cdi_cache *cache, uint64_t block, size_t count, const void *src, void *prv);

#ifdef __cplusplus
extern "C" {
#endif

struct cdi_cache *cdi_cache_create(size_t block_size, size_t blkpriv_len, cdi_cache_read_block_t *read_block, cdi_cache_write_block_t *write_block, void *prv_data);
void cdi_cache_destroy(struct cdi_cache *cache);
int cdi_cache_sync(struct cdi_cache *cache);

struct cdi_cache_block *cdi_cache_block_get(struct cdi_cache *cache, uint64_t blocknum, int noread);
void cdi_cache_block_release(struct cdi_cache *cache, struct cdi_cache_block *block);
void cdi_cache_block_dirty(struct cdi_cache *cache, struct cdi_cache_block *block);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
