// Copyright (C) 2022  ilobilo

#include <cdi/storage.h>

extern "C"
{
    void cdi_storage_driver_init(cdi_storage_driver *driver);
    void cdi_storage_driver_destroy(cdi_storage_driver *driver);
    void cdi_storage_device_init(cdi_storage_device *device);
} // extern "C"