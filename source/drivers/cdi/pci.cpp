// Copyright (C) 2022  ilobilo

#include <drivers/pci/pci.hpp>
#include <cdi/pci.h>

extern "C"
{
    void cdi_pci_get_all_devices(cdi_list_t list)
    {
        for (const auto device : pci::devices)
        {
            auto cdi_device = new cdi_pci_device
            {
                .bus_data = {
                    .bus_type = CDI_PCI
                },
                .bus = device->bus,
                .dev = device->dev,
                .function = device->func,
                .vendor_id = device->vendorid,
                .device_id = device->deviceid,
                .class_id = device->Class,
                .subclass_id = device->subclass,
                .interface_id = device->progif,
                .rev_id = device->read<uint8_t>(pci::PCI_REVISION_ID),
                .irq = device->read<uint8_t>(pci::PCI_INTERRUPT_LINE),
                .resources = cdi_list_create(),
                .meta = {
                    .seg = device->seg
                }
            };

            size_t barcount = 0;
            uint8_t hdr_type = device->read<uint8_t>(pci::PCI_HEADER_TYPE);
            if (hdr_type == 0x00) barcount = 6;
            else if (hdr_type == 0x01) barcount = 2;

            for (size_t i = 0; i < barcount; i++)
            {
                auto [type, addr, length] = device->get_bar(i);
                if (type == pci::PCI_BARTYPE_INVALID) continue;

                cdi_list_push(cdi_device->resources, new cdi_pci_resource
                {
                    .type = (type == pci::PCI_BARTYPE_MMIO ? CDI_PCI_MEMORY : CDI_PCI_IOPORTS),
                    .start = addr,
                    .length = length,
                    .index = static_cast<unsigned int>(i),
                    .address = reinterpret_cast<void*>(addr),
                });
            }

            cdi_list_push(list, cdi_device);
        }
    }

    void cdi_pci_device_destroy(cdi_pci_device *device)
    {
        while (!cdi_list_empty(device->resources))
        {
            auto res = static_cast<cdi_pci_resource*>(cdi_list_pop(device->resources));
            if (res != nullptr) delete res;
        }
        cdi_list_destroy(device->resources);
        delete device;
    }

    void cdi_pci_alloc_ioports(cdi_pci_device *device) { }
    void cdi_pci_free_ioports(cdi_pci_device *device) { }
    void cdi_pci_alloc_memory(cdi_pci_device *device) { }
    void cdi_pci_free_memory(cdi_pci_device *device) { }

    uint8_t cdi_pci_config_readb(cdi_pci_device *device, uint8_t offset)
    {
        return pci::read(device->meta.seg, device->bus, device->dev, device->function, offset, 1);
    }

    uint16_t cdi_pci_config_readw(cdi_pci_device *device, uint8_t offset)
    {
        return pci::read(device->meta.seg, device->bus, device->dev, device->function, offset, 2);
    }

    uint32_t cdi_pci_config_readl(cdi_pci_device *device, uint8_t offset)
    {
        return pci::read(device->meta.seg, device->bus, device->dev, device->function, offset, 4);
    }

    void cdi_pci_config_writeb(cdi_pci_device *device, uint8_t offset, uint8_t value)
    {
        pci::write(device->meta.seg, device->bus, device->dev, device->function, offset, value, 1);
    }

    void cdi_pci_config_writew(cdi_pci_device *device, uint8_t offset, uint16_t value)
    {
        pci::write(device->meta.seg, device->bus, device->dev, device->function, offset, value, 2);
    }

    void cdi_pci_config_writel(cdi_pci_device *device, uint8_t offset, uint32_t value)
    {
        pci::write(device->meta.seg, device->bus, device->dev, device->function, offset, value, 4);
    }
} // extern "C"