/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_NET_H_
#define _CDI_NET_H_

#include <stdint.h>
#include <stddef.h>

#include <cdi.h>

struct cdi_net_device
{
    struct cdi_device dev;
    uint64_t mac : 48;
    int number;
};

struct cdi_net_driver
{
    struct cdi_driver drv;
    void (*send_packet)(struct cdi_net_device *device, void *data, size_t size);
};

#ifdef __cplusplus
extern "C" {
#endif

void cdi_net_driver_init(struct cdi_net_driver *driver);
void cdi_net_driver_destroy(struct cdi_net_driver *driver);
void cdi_net_device_init(struct cdi_net_device *device);
void cdi_net_receive(struct cdi_net_device *device, void *buffer, size_t size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif