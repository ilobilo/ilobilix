/*
 * Copyright (c) 2008 Janosch Graef
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_SCSI_H_
#define _CDI_SCSI_H_

#include <stdint.h>
#include <stddef.h>

#include <cdi.h>

struct cdi_scsi_packet
{
    int lun;
    void *buffer;
    size_t bufsize;
    enum {
        CDI_SCSI_NODATA,
        CDI_SCSI_READ,
        CDI_SCSI_WRITE,
    } direction;
    uint8_t command[16];
    size_t cmdsize;
};

struct cdi_scsi_device
{
    struct cdi_device dev;
    cdi_device_type_t type;
    int lun_count;
};

struct cdi_scsi_driver
{
    struct cdi_driver drv;
    int (*request)(struct cdi_scsi_device *device, struct cdi_scsi_packet *packet);
};

#ifdef __cplusplus
extern "C" {
#endif

struct cdi_scsi_packet *cdi_scsi_packet_alloc(size_t size);
void cdi_scsi_packet_free(struct cdi_scsi_packet *packet);
void cdi_scsi_driver_init(struct cdi_scsi_driver *driver);
void cdi_scsi_driver_destroy(struct cdi_scsi_driver *driver);
void cdi_scsi_driver_register(struct cdi_scsi_driver *driver);
void cdi_scsi_device_init(struct cdi_scsi_device *device);

#ifdef __cplusplus
} // extern "C"
#endif

#endif