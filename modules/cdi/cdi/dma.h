/*
 * Copyright (c) 2007 Antoine Kaufmann
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_DMA_H_
#define _CDI_DMA_H_

#include <stdint.h>

#include <cdi.h>
#include <cdi-osdep.h>

struct cdi_dma_handle
{
    uint8_t channel;
    size_t length;
    uint8_t mode;
    void *buffer;
    cdi_dma_osdep meta;
};

#define CDI_DMA_MODE_READ (1 << 2)
#define CDI_DMA_MODE_WRITE (2 << 2)
#define CDI_DMA_MODE_ON_DEMAND (0 << 6)
#define CDI_DMA_MODE_SINGLE (1 << 6)
#define CDI_DMA_MODE_BLOCK (2 << 6)

#ifdef __cplusplus
extern "C" {
#endif

int cdi_dma_open(struct cdi_dma_handle *handle, uint8_t channel, uint8_t mode, size_t length, void *buffer);
int cdi_dma_read(struct cdi_dma_handle *handle);
int cdi_dma_write(struct cdi_dma_handle *handle);
int cdi_dma_close(struct cdi_dma_handle *handle);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

