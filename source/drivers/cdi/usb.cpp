// Copyright (C) 2022  ilobilo

#include <cdi/usb.h>

extern "C"
{
    void cdi_usb_driver_register(cdi_usb_driver *drv);
    void cdi_usb_get_endpoint_descriptor(cdi_usb_device *dev, int ep, cdi_usb_endpoint_descriptor *desc);

    cdi_usb_transmission_result_t cdi_usb_control_transfer(cdi_usb_device *dev, int ep, const cdi_usb_setup_packet *setup, void *data);
    cdi_usb_transmission_result_t cdi_usb_bulk_transfer(cdi_usb_device *dev, int ep, void *data, size_t size);
} // extern "C"