// Copyright (C) 2022  ilobilo

#include <cdi/storage.h>

#include <lib/log.hpp>

extern "C"
{
    void cdi_storage_driver_init(cdi_storage_driver *driver)
    {
        driver->drv.type = CDI_STORAGE;
        cdi_driver_init(reinterpret_cast<cdi_driver*>(driver));
    }

    void cdi_storage_driver_destroy(cdi_storage_driver *driver)
    {
        cdi_driver_destroy(reinterpret_cast<cdi_driver*>(driver));
    }

    void cdi_storage_device_init(cdi_storage_device *device);
} // extern "C"