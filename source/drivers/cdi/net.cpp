// Copyright (C) 2022  ilobilo

#include <cdi/net.h>

extern "C"
{
    void cdi_net_driver_init(cdi_net_driver *driver);
    void cdi_net_driver_destroy(cdi_net_driver *driver);
    void cdi_net_device_init(cdi_net_device *device);
    void cdi_net_receive(cdi_net_device *device, void *buffer, size_t size);
} // extern "C"