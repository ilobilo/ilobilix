// Copyright (C) 2022  ilobilo

#include <cdi/dma.h>

extern "C"
{
    int cdi_dma_open(cdi_dma_handle *handle, uint8_t channel, uint8_t mode, size_t length, void *buffer);
    int cdi_dma_read(cdi_dma_handle *handle);
    int cdi_dma_write(cdi_dma_handle *handle);
    int cdi_dma_close(cdi_dma_handle *handle);
} // extern "C"