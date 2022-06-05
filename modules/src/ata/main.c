/*
 * Copyright (c) 2007-2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cdi/storage.h>
#include <cdi/lists.h>
#include <cdi/pci.h>

#include "device.h"

#define DRIVER_STORAGE_NAME "ata"
#define DRIVER_SCSI_NAME "atapi"

static struct cdi_storage_driver driver_storage;
static struct cdi_scsi_driver driver_scsi;
static cdi_list_t controller_list = NULL;

static int argc;
static char **argv;

#ifdef CDI_STANDALONE
int main(int _argc, char *_argv[])
#else
int init_ata(int _argc, char *_argv[])
#endif
{
    argc = _argc;
    argv = _argv;

    cdi_init();

    return 0;
}

static int ata_driver_init(void)
{
    struct ata_controller *controller;
    uint16_t busmaster_regbase = 0;
    struct cdi_pci_device *pci_dev;
    cdi_list_t pci_devices;
    int i;
    int j;

    cdi_storage_driver_init((struct cdi_storage_driver*) &driver_storage);
    cdi_scsi_driver_init((struct cdi_scsi_driver*) &driver_scsi);

    controller_list = cdi_list_create();

    pci_devices = cdi_list_create();
    cdi_pci_get_all_devices(pci_devices);
    for (i = 0; (pci_dev = cdi_list_get(pci_devices, i)) && !busmaster_regbase; i++)
    {
        struct cdi_pci_resource *res;

        if ((pci_dev->class_id != PCI_CLASS_ATA) || (pci_dev->subclass_id != PCI_SUBCLASS_ATA)) continue;

        for (j = 0; (res = cdi_list_get(pci_dev->resources, j)); j++)
        {
            if ((res->type == CDI_PCI_IOPORTS) && (res->length == 16))
            {
                busmaster_regbase = res->start;
                break;
            }
        }
    }

    if (pci_dev && (pci_dev->vendor_id == PCI_VENDOR_VIA) && (pci_dev->device_id == 0x686)) busmaster_regbase = 0;
    else
    {
        for (i = 1; i < argc; i++)
        {
            if (!strcmp(argv[i], "nodma"))
            {
                busmaster_regbase = 0;
                break;
            }
        }
    }

    controller = calloc(1, sizeof(*controller));
    controller->port_cmd_base = ATA_PRIMARY_CMD_BASE;
    controller->port_ctl_base = ATA_PRIMARY_CTL_BASE;
    controller->port_bmr_base = busmaster_regbase;
    controller->irq = ATA_PRIMARY_IRQ;
    controller->id = 0;
    controller->storage = (struct cdi_storage_driver*)&driver_storage;
    controller->scsi = (struct cdi_scsi_driver*)&driver_scsi;
    ata_init_controller(controller);
    cdi_list_push(controller_list, controller);

    controller = calloc(1, sizeof(*controller));
    controller->port_cmd_base = ATA_SECONDARY_CMD_BASE;
    controller->port_ctl_base = ATA_SECONDARY_CTL_BASE;
    controller->port_bmr_base = busmaster_regbase;
    controller->irq = ATA_SECONDARY_IRQ;
    controller->id = 1;
    controller->storage = (struct cdi_storage_driver*)&driver_storage;
    controller->scsi = (struct cdi_scsi_driver*)&driver_scsi;
    ata_init_controller(controller);
    cdi_list_push(controller_list, controller);

    return 0;
}

static int ata_driver_destroy(void)
{
    cdi_storage_driver_destroy(&driver_storage);
    return 0;
}

static int atapi_driver_init(void)
{
    return 0;
}

static int atapi_driver_destroy(void)
{
    cdi_scsi_driver_destroy(&driver_scsi);
    return 0;
}

static struct cdi_storage_driver driver_storage = {
    .drv = {
        .type           = CDI_STORAGE,
        .name           = DRIVER_STORAGE_NAME,
        .init           = ata_driver_init,
        .destroy        = ata_driver_destroy,
        .remove_device  = ata_remove_device,
    },
    .read_blocks        = ata_read_blocks,
    .write_blocks       = ata_write_blocks,
};

static struct cdi_scsi_driver driver_scsi = {
    .drv = {
        .type           = CDI_SCSI,
        .name           = DRIVER_SCSI_NAME,
        .init           = atapi_driver_init,
        .destroy        = atapi_driver_destroy,
        .remove_device  = atapi_remove_device,
    },
    .request            = atapi_request,
};

CDI_DRIVER(DRIVER_STORAGE_NAME, driver_storage)
CDI_DRIVER(DRIVER_SCSI_NAME, driver_scsi)