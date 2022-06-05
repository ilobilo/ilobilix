// Copyright (C) 2022  ilobilo

#include <drivers/elf/elf.hpp>
#include <lib/log.hpp>
#include <cdi/pci.h>

extern "C"
{
    static cdi_list_t drivers = nullptr;

    void pcidevices_init()
    {
        cdi_list_t pcidevices = cdi_list_create();
        cdi_pci_get_all_devices(pcidevices);

        while (!cdi_list_empty(pcidevices))
        {
            auto pcidevice = static_cast<cdi_pci_device*>(cdi_list_pop(pcidevices));
            if (pcidevice == nullptr) continue;

            cdi_device *device = nullptr;
            for (size_t i = 0; i < cdi_list_size(drivers); i++)
            {
                auto driver = static_cast<cdi_driver*>(cdi_list_get(drivers, i));
                if (driver == nullptr) continue;

                if (driver->bus == CDI_PCI && driver->init_device != nullptr)
                {
                    device = driver->init_device(&pcidevice->bus_data);
                    if (device != nullptr)
                    {
                        cdi_list_push(driver->devices, device);
                        break;
                    }
                }
            }
            if (device == nullptr) cdi_pci_device_destroy(pcidevice);
        }

        cdi_list_destroy(pcidevices);
    }

    void run_all()
    {
        pcidevices_init();

        for (size_t i = 0; i < cdi_list_size(drivers); i++)
        {
            auto driver = static_cast<cdi_driver*>(cdi_list_get(drivers, i));
            if (driver == nullptr || driver->devices == nullptr) continue;

            for (size_t t = 0; t < cdi_list_size(driver->devices); t++)
            {
                auto device = static_cast<cdi_device*>(cdi_list_get(driver->devices, i));
                if (device == nullptr) continue;

                device->driver = driver;
            }
        }
    }

    void cdi_init()
    {
        drivers = cdi_list_create();

        for (const auto module : elf::module::modules)
        {
            for (const auto driver : module->drivers)
            {
                if (driver->init != nullptr)
                {
                    driver->init();
                    cdi_driver_register(driver);
                }
            }
        }

        run_all();
    }

    void cdi_driver_init(cdi_driver *driver)
    {
        driver->devices = cdi_list_create();
    }

    void cdi_driver_destroy(cdi_driver *driver)
    {
        while (!cdi_list_empty(driver->devices))
        {
            auto dev = static_cast<cdi_device*>(cdi_list_pop(driver->devices));
            if (dev == nullptr) continue;

            if (driver->remove_device != nullptr) driver->remove_device(dev);
            delete dev;
        }

        cdi_list_destroy(driver->devices);
    }

    void cdi_driver_register(cdi_driver *driver)
    {
        log::info("CDI: Registering driver: \"%s\"", driver->name);
        cdi_list_push(drivers, driver);

        switch (driver->type)
        {
            default:
                log::error("CDI: Unsupported driver type %d!", driver->type);
                break;
        }
    }

    int cdi_provide_device(cdi_bus_data *device);
    int cdi_provide_device_internal_drv(cdi_bus_data *device, cdi_driver *driver);
    void cdi_handle_bus_device(cdi_driver *drv, cdi_bus_device_pattern *pattern);
} // extern "C"