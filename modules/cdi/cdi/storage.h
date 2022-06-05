/*
 * Copyright (c) 2007 Antoine Kaufmann
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_STORAGE_H_
#define _CDI_STORAGE_H_

#include <stdint.h>
#include <cdi.h>

struct cdi_storage_device
{
    struct cdi_device dev;
    size_t block_size;
    uint64_t block_count;
};

struct cdi_storage_driver
{
    struct cdi_driver drv;

    int (*read_blocks)(struct cdi_storage_device *device, uint64_t start, uint64_t count, void *buffer);
    int (*write_blocks)(struct cdi_storage_device *device, uint64_t start, uint64_t count, void *buffer);
};

#ifdef __cplusplus
extern "C" {
#endif

void cdi_storage_driver_init(struct cdi_storage_driver *driver);
void cdi_storage_driver_destroy(struct cdi_storage_driver *driver);
void cdi_storage_device_init(struct cdi_storage_device *device);

#ifdef __cplusplus
} // extern "C"
#endif

#endif