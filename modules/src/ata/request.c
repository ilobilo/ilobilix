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
#include <string.h>

#include <cdi.h>
#include <cdi/storage.h>
#include <cdi/misc.h>
#include <cdi/io.h>

#include "device.h"

static inline int ata_drv_wait_ready(struct ata_device *dev, uint8_t bits, uint32_t timeout)
{
    struct ata_controller *ctrl = dev->controller;
    uint32_t time = 0;

    ATA_DELAY(dev->controller);

    while (((ata_reg_inb(ctrl, REG_STATUS) & STATUS_BSY)) && (time < timeout))
    {
        time += 10;
        cdi_sleep_ms(10);
    }

    ATA_DELAY(dev->controller);

    while (((ata_reg_inb(ctrl, REG_STATUS) & bits) != bits) && (time < timeout))
    {
        time += 10;
        cdi_sleep_ms(10);
    }

    return (time < timeout);
}

static int ata_request_command(struct ata_request *request)
{
    struct ata_device *dev = request->dev;
    struct ata_controller *ctrl = dev->controller;
    uint8_t control;

    cdi_reset_wait_irq(ctrl->irq);

    ata_drv_select(dev);

    if (!ata_drv_wait_ready(dev, 0, ATA_READY_TIMEOUT))
    {
        request->error = DEVICE_READY_TIMEOUT;
        return 0;
    }

    ata_reg_outb(ctrl, REG_DEVICE, (request->flags.lba << 6) | (request->dev->id << 4) | ((request->registers.ata.lba >> 24) & 0xF));

    control = 0;
    if (request->flags.poll) control |= CONTROL_NIEN;

    ata_reg_outb(ctrl, REG_CONTROL, control);
    ata_reg_outb(ctrl, REG_FEATURES, request->registers.ata.features);
    ata_reg_outb(ctrl, REG_SEC_CNT, request->registers.ata.count);
    ata_reg_outb(ctrl, REG_LBA_LOW, request->registers.ata.lba & 0xFF);
    ata_reg_outb(ctrl, REG_LBA_MID, (request->registers.ata.lba >> 8) & 0xFF);
    ata_reg_outb(ctrl, REG_LBA_HIG, (request->registers.ata.lba >> 16) & 0xFF);
    ata_reg_outb(ctrl, REG_COMMAND, request->registers.ata.command);

    return 1;
}

static int ata_protocol_non_data(struct ata_request *request)
{
    struct ata_device *dev = request->dev;
    struct ata_controller *ctrl = dev->controller;

    enum
    {
        IRQ_WAIT,
        CHECK_STATUS
    } state;

    if (request->flags.poll) state = CHECK_STATUS;
    else state = IRQ_WAIT;

    while (1)
    {
        switch (state)
        {
            case IRQ_WAIT:
                if (!ata_wait_irq(ctrl, ATA_IRQ_TIMEOUT))
                {
                    request->error = IRQ_TIMEOUT;
                    DEBUG("non_data IRQ-Timeout\n");
                    return 0;
                }
                state = CHECK_STATUS;
                break;
            case CHECK_STATUS:
                if ((ata_reg_inb(ctrl, REG_STATUS) & STATUS_BSY) == STATUS_BSY) cdi_sleep_ms(20);
                else return 1;
                break;
        }
    }
}

int ata_protocol_pio_in(struct ata_request *request)
{
    struct ata_device *dev = request->dev;
    struct ata_controller *ctrl = dev->controller;
    size_t packet_size = 0;

    enum
    {
        IRQ_WAIT,
        CHECK_STATUS,
        TRANSFER_DATA
    } state;

    if (request->flags.poll) state = CHECK_STATUS;
    else state = IRQ_WAIT;

    while (1)
    {
        switch (state)
        {
            case IRQ_WAIT:
                if (!ata_wait_irq(ctrl, ATA_IRQ_TIMEOUT))
                {
                    request->error = IRQ_TIMEOUT;
                    DEBUG("pio_in IRQ-Timeout\n");
                    return 0;
                }

                if (request->flags.ata && packet_size == 0)
                {
                    packet_size = ata_reg_inb(ctrl, REG_LBA_MID) | (ata_reg_inb(ctrl, REG_LBA_HIG) << 8);
                }

                state = CHECK_STATUS;
                break;
            case CHECK_STATUS:
            {
                uint8_t status = ata_reg_inb(ctrl, REG_STATUS);

                if ((status & (STATUS_BSY | STATUS_DRQ)) == 0)
                {
                    DEBUG("pio_in unerwarteter Status: 0x%x\n", status);
                    return 0;
                }
                else if ((status & STATUS_BSY) == STATUS_BSY) ATA_DELAY(ctrl);
                else if ((status & (STATUS_BSY | STATUS_DRQ)) == STATUS_DRQ) state = TRANSFER_DATA;
                break;
            }
            case TRANSFER_DATA:
            {
                uint16_t *buffer = (uint16_t*)(request->buffer + (request->blocks_done  *request->block_size));

                if (!request->flags.poll) cdi_reset_wait_irq(ctrl->irq);

                ata_insw(ata_reg_base(ctrl, REG_DATA) + REG_DATA, buffer, request->block_size / 2);
                request->blocks_done++;

                if (!request->flags.ata && request->blocks_done >= request->block_count) return 1;
                else if (request->flags.ata && packet_size && request->blocks_done*request->block_size>=packet_size) return 1;
                else if (request->flags.poll) state = CHECK_STATUS;
                else state = IRQ_WAIT;
            }
        }
    }
}

int ata_protocol_pio_out(struct ata_request *request)
{
    struct ata_device *dev = request->dev;
    struct ata_controller *ctrl = dev->controller;
    size_t packet_size = 0;

    enum
    {
        IRQ_WAIT,
        CHECK_STATUS,
        TRANSFER_DATA
    } state;

    state = CHECK_STATUS;
    while (1)
    {
        switch (state)
        {
            case IRQ_WAIT:
                if (!ata_wait_irq(ctrl, ATA_IRQ_TIMEOUT))
                {
                    request->error = IRQ_TIMEOUT;
                    DEBUG("pio_out IRQ-Timeout\n");
                    return 0;
                }

                if (request->flags.ata && packet_size == 0)
                {
                    packet_size = ata_reg_inb(ctrl, REG_LBA_MID) | (ata_reg_inb(ctrl,REG_LBA_HIG) << 8);
                    DEBUG("packet_size = %d\n",packet_size);
                }

                state = CHECK_STATUS;
                break;

            case CHECK_STATUS:
            {
                uint8_t status = ata_reg_inb(ctrl, REG_STATUS);

                if (request->flags.ata && request->blocks_done  *request->block_size>=packet_size) return 1;
                else if (!request->flags.ata && request->blocks_done==request->block_count) return 1;
                else if ((status & (STATUS_BSY | STATUS_DRQ)) == 0)
                {
                    DEBUG("pio_out unexpected status: 0x%x\n", status);
                    return 0;
                }
                else if ((status & STATUS_BSY) == STATUS_BSY) ATA_DELAY(ctrl);
                else if ((status & (STATUS_BSY | STATUS_DRQ)) == STATUS_DRQ) state = TRANSFER_DATA;
                break;
            }
            case TRANSFER_DATA:
            {
                uint16_t *buffer = (uint16_t*) (request->buffer + (request->blocks_done  *request->block_size));

                if (!request->flags.poll) cdi_reset_wait_irq(ctrl->irq);

                ata_outsw(ata_reg_base(ctrl, REG_DATA) + REG_DATA, buffer, request->block_size / 2);
                request->blocks_done++;

                if (request->flags.poll) state = CHECK_STATUS;
                else state = IRQ_WAIT;
            }
        }
    }
}

static int ata_request_dma_init(struct ata_request *request)
{
    struct ata_device *dev = request->dev;
    struct ata_controller *ctrl = dev->controller;
    uint64_t size = request->block_size  *request->block_count;

    *ctrl->prdt_virt = (uint32_t)ctrl->dma_buf_phys;
    *ctrl->prdt_virt |= (size & (ATA_DMA_MAXSIZE - 1)) << 32L;
    *ctrl->prdt_virt |= (uint64_t)1L << 63L;

    cdi_outb(ctrl->port_bmr_base + BMR_COMMAND, 0);
    cdi_outb(ctrl->port_bmr_base + BMR_STATUS, cdi_inb(ctrl->port_bmr_base + BMR_STATUS) | BMR_STATUS_ERROR | BMR_STATUS_IRQ);
    cdi_outl(ctrl->port_bmr_base + BMR_PRDT, ctrl->prdt_phys);

    if (request->flags.direction != READ) memcpy(ctrl->dma_buf_virt, request->buffer, size);
    return 1;
}

static int ata_protocol_dma(struct ata_request *request)
{
    struct ata_device *dev = request->dev;
    struct ata_controller *ctrl = dev->controller;

    enum
    {
        IRQ_WAIT,
        CHECK_STATUS,
    } state;

    cdi_inb(ctrl->port_bmr_base + BMR_COMMAND);
    cdi_inb(ctrl->port_bmr_base + BMR_STATUS);

    if (request->flags.direction == READ) cdi_outb(ctrl->port_bmr_base + BMR_COMMAND, BMR_CMD_START | BMR_CMD_READ);
    else cdi_outb(ctrl->port_bmr_base + BMR_COMMAND, BMR_CMD_START);
    cdi_inb(ctrl->port_bmr_base + BMR_COMMAND);
    cdi_inb(ctrl->port_bmr_base + BMR_STATUS);

    if (request->flags.poll) state = CHECK_STATUS;
    else state = IRQ_WAIT;

    while (1)
    {
        switch (state)
        {
            case IRQ_WAIT:
                if (!ata_wait_irq(ctrl, ATA_IRQ_TIMEOUT))
                {
                    request->error = IRQ_TIMEOUT;
                    DEBUG("dma IRQ-Timeout\n");
                    return 0;
                }

                state = CHECK_STATUS;
                break;
            case CHECK_STATUS:
            {
                uint8_t status = ata_reg_inb(ctrl, REG_STATUS);

                if ((status & (STATUS_BSY | STATUS_DRQ)) == 0)
                {
                    cdi_inb(ctrl->port_bmr_base + BMR_STATUS);
                    cdi_outb(ctrl->port_bmr_base + BMR_COMMAND, 0);
                    goto out_success;
                }
                break;
            }
        }
    }

out_success:
    if (request->flags.direction == READ) memcpy(request->buffer, ctrl->dma_buf_virt, request->block_size  *request->block_count);
    return 1;
}

int ata_request(struct ata_request *request)
{
    if ((request->protocol == DMA) && !ata_request_dma_init(request))
    {
        DEBUG("Failed to initialize DMA-Controllers\n");
        return 0;
    }

    if (!ata_request_command(request))
    {
        DEBUG("Error executing command\n");
        return 0;
    }

    switch (request->protocol)
    {
        case NON_DATA:
            if (!ata_protocol_non_data(request)) return 0;
            break;
        case PIO:
            if ((request->flags.direction == READ) && (!ata_protocol_pio_in(request))) return 0;
            else if ((request->flags.direction == WRITE) && (!ata_protocol_pio_out(request))) return 0;
            break;
        case DMA:
            if (!ata_protocol_dma(request)) return 0;
            break;
        default:
            return 0;
    }
    return 1;
}