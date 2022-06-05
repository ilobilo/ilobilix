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

#include <stdio.h>
#include <stdlib.h>

#include <cdi.h>
#include <cdi/storage.h>
#include <cdi/misc.h>
#include <cdi/io.h>
#include <cdi/scsi.h>
#include <cdi/mem.h>

#include "device.h"

static inline void ata_controller_irq(struct cdi_device *dev);

static int ata_bus_floating(struct ata_controller *controller)
{
    uint8_t status;

    ata_reg_outb(controller, REG_DEVICE, DEVICE_DEV(0));
    ATA_DELAY(controller);
    status = ata_reg_inb(controller, REG_STATUS);

    if (status != 0xFF) return 0;

    ata_reg_outb(controller, REG_DEVICE, DEVICE_DEV(1));
    ATA_DELAY(controller);
    status = ata_reg_inb(controller, REG_STATUS);

    return (status == 0xFF);
}

static int ata_bus_responsive_drv(struct ata_controller *controller)
{
    ata_reg_outb(controller, REG_DEVICE, DEVICE_DEV(1));
    ATA_DELAY(controller);

    ata_reg_outb(controller, REG_LBA_LOW, 0xAF);
    ata_reg_outb(controller, REG_LBA_MID, 0xBF);
    ata_reg_outb(controller, REG_LBA_HIG, 0xCF);

    if ((ata_reg_inb(controller, REG_LBA_LOW) == 0xAF) && (ata_reg_inb(controller, REG_LBA_MID) == 0xBF) && (ata_reg_inb(controller, REG_LBA_HIG) == 0xCF)) return 1;

    ata_reg_outb(controller, REG_DEVICE, DEVICE_DEV(0));
    ATA_DELAY(controller);

    ata_reg_outb(controller, REG_LBA_LOW, 0xAF);
    ata_reg_outb(controller, REG_LBA_MID, 0xBF);
    ata_reg_outb(controller, REG_LBA_HIG, 0xCF);

    if ((ata_reg_inb(controller, REG_LBA_LOW) == 0xAF) && (ata_reg_inb(controller, REG_LBA_MID) == 0xBF) && (ata_reg_inb(controller, REG_LBA_HIG) == 0xCF)) return 1;

    return 0;
}

static void ata_controller_irq(struct cdi_device *dev)
{
}

int ata_wait_irq(struct ata_controller *controller, uint32_t timeout)
{
    if (cdi_wait_irq(controller->irq, timeout)) return 0;

    return 1;
}

void ata_init_controller(struct ata_controller *controller)
{
    int i;

    if (cdi_ioports_alloc(controller->port_cmd_base, 8) != 0)
    {
        DEBUG("Error allocating I/O-Ports\n");
        return;
    }
    if (cdi_ioports_alloc(controller->port_ctl_base, 1) != 0)
    {
        DEBUG("Error allocating I/O-Ports\n");
        goto error_free_cmdbase;
    }
    if (controller->port_bmr_base && (cdi_ioports_alloc(controller->port_bmr_base, 8) != 0))
    {
        DEBUG("Error allocating I/O-Ports\n");
        goto error_free_ctlbase;
    }

    controller->irq_dev.controller = controller;
    cdi_register_irq(controller->irq, ata_controller_irq, (struct cdi_device*)&controller->irq_dev);

    for (i = 0; i < 2; i++)
    {
        ata_reg_outb(controller, REG_DEVICE, DEVICE_DEV(i));
        ATA_DELAY(controller);

        ata_reg_outb(controller, REG_CONTROL, CONTROL_NIEN);
    }

    if (ata_bus_floating(controller))
    {
        DEBUG("Floating Bus %d\n", controller->id);
        return;
    }

    if (!ata_bus_responsive_drv(controller))
    {
        DEBUG("No responsive drive on Bus %d\n", controller->id);
        return;
    }

    for (i = 1; i >= 0; i--)
    {
        struct ata_device *dev = calloc(1, sizeof(*dev));
        dev->controller = controller;
        dev->id = i;
        dev->partition_list = cdi_list_create();

        if (ata_drv_identify(dev))
        {
            DEBUG("Bus %d  Device %d: ATAPI=%d\n", (uint32_t)controller->id, i, dev->atapi);

#ifdef ATAPI_ENABLE
            if (dev->atapi == 0)
            {
#endif
                dev->dev.storage.block_size = ATA_SECTOR_SIZE;

                dev->read_sectors = ata_drv_read_sectors;
                dev->write_sectors = ata_drv_write_sectors;

                asprintf((char**)&(dev->dev.storage.dev.name), "ATA%01d%01d", (uint32_t)controller->id, i);

                dev->dev.storage.dev.driver = &controller->storage->drv;
                ata_init_device(dev);
                cdi_list_push(controller->storage->drv.devices, dev);
#ifdef ATAPI_ENABLE
            }
            else
            {
                asprintf((char**)&(dev->dev.scsi.dev.name),"ATAPI%01d%01d", (uint32_t)controller->id, i);

                dev->dev.scsi.dev.driver = &controller->scsi->drv;
                atapi_init_device(dev);
                cdi_list_push(controller->scsi->drv.devices, dev);
            }
#endif
        }
        else free(dev);
    }

    if (controller->port_bmr_base)
    {
        struct cdi_mem_area *buf;

        buf = cdi_mem_alloc(sizeof(uint64_t), CDI_MEM_PHYS_CONTIGUOUS | CDI_MEM_DMA_4G);
        controller->prdt_virt = buf->vaddr;
        controller->prdt_phys = buf->paddr.items[0].start;

        buf = cdi_mem_alloc(ATA_DMA_MAXSIZE, CDI_MEM_PHYS_CONTIGUOUS | CDI_MEM_DMA_4G | 16);
        controller->dma_buf_virt = buf->vaddr;
        controller->dma_buf_phys = buf->paddr.items[0].start;

        controller->dma_use = 1;
    }

    return;

error_free_ctlbase:
    cdi_ioports_free(controller->port_ctl_base, 1);
error_free_cmdbase:
    cdi_ioports_free(controller->port_cmd_base, 8);
}

void ata_remove_controller(struct ata_controller *controller)
{
}

void ata_init_device(struct ata_device *dev)
{
    cdi_storage_device_init(&dev->dev.storage);
}

void ata_remove_device(struct cdi_device *device)
{
}

int ata_read_blocks(struct cdi_storage_device *device, uint64_t block, uint64_t count, void *buffer)
{
    struct ata_device *dev = (struct ata_device*)device;
    struct ata_partition *partition = NULL;

    if (dev->controller == NULL)
    {
        partition = (struct ata_partition*) dev;
        dev = partition->realdev;
    }

    if (dev->read_sectors == NULL) return -1;

    if (partition == NULL) return !dev->read_sectors(dev, block, count, buffer);
    else return !dev->read_sectors(dev, block + partition->start, count, buffer);
}

int ata_write_blocks(struct cdi_storage_device *device, uint64_t block, uint64_t count, void *buffer)
{
    struct ata_device *dev = (struct ata_device*)device;
    struct ata_partition *partition = NULL;

    if (dev->controller == NULL)
    {
        partition = (struct ata_partition*)dev;
        dev = partition->realdev;
    }

    if (dev->write_sectors == NULL) return -1;

    if (partition == NULL) return !dev->write_sectors(dev, block, count, buffer);
    else return !dev->write_sectors(dev, block + partition->start, count, buffer);
}