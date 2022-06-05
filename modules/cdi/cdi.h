/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_H_
#define _CDI_H_

#include <stdint.h>
#include <stdbool.h>

#include <cdi-osdep.h>
#include <cdi/lists.h>

typedef enum
{
    CDI_UNKNOWN = 0,
    CDI_NETWORK = 1,
    CDI_STORAGE = 2,
    CDI_SCSI = 3,
    CDI_VIDEO = 4,
    CDI_AUDIO = 5,
    CDI_AUDIO_MIXER = 6,
    CDI_USB_HCD = 7,
    CDI_USB = 8,
    CDI_FILESYSTEM = 9,
    CDI_PCI = 10,
    CDI_AHCI = 11,
} cdi_device_type_t;

struct cdi_driver;
struct cdi_bus_data
{
    cdi_device_type_t bus_type;
};

struct cdi_bus_device_pattern
{
    cdi_device_type_t bus_type;
};

struct cdi_device
{
    const char *name;
    struct cdi_driver *driver;
    struct cdi_bus_data *bus_data;
};

struct cdi_driver
{
    cdi_device_type_t type;
    cdi_device_type_t bus;
    const char *name;
    bool initialised;
    cdi_list_t devices;

    struct cdi_device *(*init_device)(struct cdi_bus_data *bus_data);
    void (*remove_device)(struct cdi_device *device);

    int (*init)(void);
    int (*destroy)(void);
};

#ifdef __cplusplus
extern "C" {
#endif

void cdi_init(void);
void cdi_driver_init(struct cdi_driver *driver);
void cdi_driver_destroy(struct cdi_driver *driver);
void cdi_driver_register(struct cdi_driver *driver);
int cdi_provide_device(struct cdi_bus_data *device);
int cdi_provide_device_internal_drv(struct cdi_bus_data *device, struct cdi_driver *driver);
void cdi_handle_bus_device(struct cdi_driver *drv, struct cdi_bus_device_pattern *pattern);

#ifdef __cplusplus
}
#endif

#endif