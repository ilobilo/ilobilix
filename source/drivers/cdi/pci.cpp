// Copyright (C) 2022  ilobilo

#include <cdi/pci.h>

extern "C"
{
    void cdi_pci_get_all_devices(cdi_list_t list);
    void cdi_pci_device_destroy(cdi_pci_device *device);
    void cdi_pci_alloc_ioports(cdi_pci_device *device);
    void cdi_pci_free_ioports(cdi_pci_device *device);
    void cdi_pci_alloc_memory(cdi_pci_device *device);
    void cdi_pci_free_memory(cdi_pci_device *device);

    uint8_t cdi_pci_config_readb(cdi_pci_device *device, uint8_t offset);
    uint16_t cdi_pci_config_readw(cdi_pci_device *device, uint8_t offset);
    uint32_t cdi_pci_config_readl(cdi_pci_device *device, uint8_t offset);

    void cdi_pci_config_writeb(cdi_pci_device *device, uint8_t offset, uint8_t value);
    void cdi_pci_config_writew(cdi_pci_device *device, uint8_t offset, uint16_t value);
    void cdi_pci_config_writel(cdi_pci_device *device, uint8_t offset, uint32_t value);
} // extern "C"