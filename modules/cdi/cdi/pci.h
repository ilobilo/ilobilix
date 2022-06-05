/*
 * Copyright (c) 2007-2010 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_PCI_H_
#define _CDI_PCI_H_

#include <stdint.h>

#include <cdi.h>
#include <cdi-osdep.h>
#include <cdi/lists.h>

#define PCI_CLASS_STORAGE 0x01
#define PCI_SUBCLASS_ST_SATA 0x06

#define PCI_CLASS_MULTIMEDIA 0x04
#define PCI_SUBCLASS_MM_HDAUDIO 0x03

struct cdi_pci_device
{
    struct cdi_bus_data bus_data;
    uint16_t bus;
    uint16_t dev;
    uint16_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_id;
    uint8_t subclass_id;
    uint8_t interface_id;
    uint8_t rev_id;
    uint8_t irq;
    cdi_list_t resources;
    cdi_pci_device_osdep meta;
};

typedef enum
{
    CDI_PCI_MEMORY,
    CDI_PCI_IOPORTS
} cdi_res_t;

struct cdi_pci_resource
{
    cdi_res_t type;
    uintptr_t start;
    size_t length;
    unsigned int index;
    void *address;
};

#ifdef __cplusplus
extern "C" {
#endif

void cdi_pci_get_all_devices(cdi_list_t list);
void cdi_pci_device_destroy(struct cdi_pci_device *device);
void cdi_pci_alloc_ioports(struct cdi_pci_device *device);
void cdi_pci_free_ioports(struct cdi_pci_device *device);
void cdi_pci_alloc_memory(struct cdi_pci_device *device);
void cdi_pci_free_memory(struct cdi_pci_device *device);

#define CDI_PCI_DIRECT_ACCESS

uint8_t cdi_pci_config_readb(struct cdi_pci_device *device, uint8_t offset);
uint16_t cdi_pci_config_readw(struct cdi_pci_device *device, uint8_t offset);
uint32_t cdi_pci_config_readl(struct cdi_pci_device *device, uint8_t offset);

void cdi_pci_config_writeb(struct cdi_pci_device *device, uint8_t offset, uint8_t value);
void cdi_pci_config_writew(struct cdi_pci_device *device, uint8_t offset, uint16_t value);
void cdi_pci_config_writel(struct cdi_pci_device *device, uint8_t offset, uint32_t value);

#ifdef __cplusplus
} // extern "C"
#endif

#endif