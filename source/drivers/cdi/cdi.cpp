// Copyright (C) 2022  ilobilo

#include <cdi.h>

extern "C"
{
    void cdi_init();
    void cdi_driver_init(cdi_driver *driver);
    void cdi_driver_destroy(cdi_driver *driver);
    void cdi_driver_register(cdi_driver *driver);
    int cdi_provide_device(cdi_bus_data *device);
    int cdi_provide_device_internal_drv(cdi_bus_data *device, cdi_driver *driver);
    void cdi_handle_bus_device(cdi_driver *drv, cdi_bus_device_pattern *pattern);
} // extern "C"