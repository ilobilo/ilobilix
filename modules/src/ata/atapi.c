/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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
#include <string.h>

#include <cdi.h>
#include <cdi/storage.h>
#include <cdi/misc.h>
#include <cdi/io.h>
#include <cdi/scsi.h>

#include "device.h"

int atapi_drv_identify(struct ata_device *dev)
{
    struct ata_identfiy_data id;

    struct ata_request request = {
        .dev = dev,

        .flags.direction = READ,
        .flags.poll = 1,
        .flags.lba = 0,

        .protocol = PIO,
        .registers.ata.command = IDENTIFY_PACKET_DEVICE,
        .block_count = 1,
        .block_size = ATA_SECTOR_SIZE,
        .buffer = &id,

        .error = 0
    };

    if (!ata_request(&request)) return 0;
    dev->atapi = 1;

    return 1;
}

void atapi_init_device(struct ata_device *device)
{
    struct cdi_scsi_device *scsi = (struct cdi_scsi_device*) device;

    scsi->type = CDI_STORAGE;
    cdi_scsi_device_init(scsi);
}

void atapi_remove_device(struct cdi_device *device)
{
}

int atapi_request(struct cdi_scsi_device *scsi,struct cdi_scsi_packet *packet)
{
    uint8_t atapi_request[12] = { 0 };
    if (packet->cmdsize > 12) return -1;

    memcpy(atapi_request, packet->command, packet->cmdsize);

    struct ata_device *dev = (struct ata_device*)scsi;
    struct ata_request request = {
        .dev = dev,
        .protocol = PIO,
        .flags = {
            .direction = WRITE,
            .poll = 1,
            .ata = 0,
            .lba = 0
        },
        .registers = {
            .ata = {
                .command = PACKET,
                .lba = 0x00FFFF00
            }
        },
        .block_count = 1,
        .block_size = sizeof(atapi_request),
        .blocks_done = 0,
        .buffer = atapi_request,
        .error = 0
    };

    if (ata_request(&request))
    {
        int status;
        struct ata_request rw_request = {
            .dev = dev,
            .flags = {
                .poll = 1,
                .lba = 1,
                .ata = 1
            },
            .block_count = packet->bufsize / 2048,
            .block_size = 2048,
            .blocks_done = 0,
            .buffer = packet->buffer,
            .error = 0
        };

        if (packet->bufsize < 2048)
        {
            rw_request.block_count = 1;
            rw_request.block_size = packet->bufsize;
        }

        if (packet->direction == CDI_SCSI_READ)
        {
            ata_protocol_pio_in(&rw_request);
        } else if (packet->direction == CDI_SCSI_WRITE)
        {
            ata_protocol_pio_out(&rw_request);
        }

        status = ata_reg_inb(dev->controller, REG_STATUS);

        if (status & STATUS_ERR) return (ata_reg_inb(dev->controller, REG_ERROR) >> 4);
        else return 0;
    }

    return 0xB;
}