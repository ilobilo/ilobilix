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

#include <cdi/storage.h>
#include <cdi/misc.h>
#include <cdi/io.h>
#include <cdi.h>

#include "device.h"

int ata_drv_identify(struct ata_device *dev)
{
    struct ata_identfiy_data id;

    struct ata_request request = {
        .dev = dev,

        .flags.direction = READ,
        .flags.poll = 1,
        .flags.lba = 0,

        .protocol = PIO,
        .registers.ata.command = IDENTIFY_DEVICE,
        .block_count = 1,
        .block_size = ATA_SECTOR_SIZE,
        .buffer = &id,

        .error = 0
    };

    if (!ata_request(&request)) return atapi_drv_identify(dev);

    if (id.features_support.bits.lba48)
    {
        dev->lba48 = 1;
        dev->dev.storage.block_count = id.max_lba48_address;
    }
    if (id.capabilities.lba)
    {
        dev->lba28 = 1;
        dev->dev.storage.block_count = id.lba_sector_count;
    }

    if (id.capabilities.dma) dev->dma = 1;

    if (!dev->lba48 && !dev->lba28) return 0;

    dev->atapi = 0;

    return 1;
}

static int ata_drv_rw_sectors(struct ata_device *dev, int direction, uint64_t start, size_t count, void *buffer)
{
    int result = 1;
    struct ata_request request;

    uint16_t current_count;
    void* current_buffer = buffer;
    uint64_t lba = start;
    int max_count;
    int again = 2;
    size_t count_left = count;

    request.dev = dev;
    request.flags.poll = 0;
    request.flags.ata = 0;
    request.flags.lba = 1;

    if (direction == 0) request.flags.direction = READ;
    else request.flags.direction = WRITE;

    if (dev->dma && dev->controller->dma_use)
    {
        max_count = ATA_DMA_MAXSIZE / ATA_SECTOR_SIZE;
        request.registers.ata.command = (direction ? WRITE_SECTORS_DMA : READ_SECTORS_DMA);
        request.protocol = DMA;
    }
    else
    {
        max_count = 256;
        request.registers.ata.command = (direction ? WRITE_SECTORS : READ_SECTORS);
        request.protocol = PIO;

        request.flags.poll = 1;
    }

    while (count_left > 0)
    {
        if ((int)count_left > max_count) current_count = max_count;
        else current_count = count_left;

        request.registers.ata.count = (uint8_t) current_count;
        request.registers.ata.lba = lba;

        request.block_count = current_count;
        request.block_size = ATA_SECTOR_SIZE;
        request.blocks_done = 0;
        request.buffer = current_buffer;

        request.error = NO_ERROR;

        // TODO: LBA48
        // TODO: CHS

        if (!ata_request(&request))
        {
            if (again)
            {
                again--;
                continue;
            }
            result = 0;
            break;
        }

        current_buffer += current_count * ATA_SECTOR_SIZE;
        count_left -= current_count;
        lba += current_count;
        again = 2;
    }

    return result;
}

int ata_drv_read_sectors(struct ata_device *dev, uint64_t start, size_t count, void *buffer)
{
    return ata_drv_rw_sectors(dev, 0, start, count, buffer);
}

int ata_drv_write_sectors(struct ata_device *dev, uint64_t start, size_t count, void *buffer)
{
    return ata_drv_rw_sectors(dev, 1, start, count, buffer);
}