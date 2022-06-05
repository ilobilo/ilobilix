/*
 * Copyright (c) 2015 Max Reitz
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_USB_H_
#define _CDI_USB_H_

#include <stdint.h>

#include <cdi.h>
#include <cdi-osdep.h>
#include <cdi/usb-structures.h>

struct cdi_usb_hc;

typedef enum
{
    CDI_USB_CONTROL,
    CDI_USB_BULK,
    CDI_USB_INTERRUPT,
    CDI_USB_ISOCHRONOUS,
} cdi_usb_endpoint_type_t;

typedef enum
{
    CDI_USB_LOW_SPEED,
    CDI_USB_FULL_SPEED,
    CDI_USB_HIGH_SPEED,
    CDI_USB_SUPERSPEED,
} cdi_usb_speed_t;

typedef enum
{
    CDI_USB_OK = 0,
    CDI_USB_DRIVER_ERROR,
    CDI_USB_TIMEOUT,
    CDI_USB_BABBLE,
    CDI_USB_STALL,
    CDI_USB_HC_ERROR,
    CDI_USB_BAD_RESPONSE,
} cdi_usb_transmission_result_t;

typedef uint32_t cdi_usb_port_status_t;

#define CDI_USB_PORT_SPEED_MASK (0x7)
#define CDI_USB_PORT_CONNECTED (1 << 3)

struct cdi_usb_hub;
struct cdi_usb_hub_driver
{
    void (*port_disable)(struct cdi_usb_hub *hub, int index);
    void (*port_reset_enable)(struct cdi_usb_hub *hub, int index);
    cdi_usb_port_status_t (*port_status)(struct cdi_usb_hub *hub, int index);
};

struct cdi_usb_hub
{
    int ports;
};

struct cdi_usb_device
{
    struct cdi_bus_data bus_data;
    uint16_t vendor_id, product_id;
    uint8_t class_id, subclass_id, protocol_id;
    int interface;
    int endpoint_count;
    cdi_usb_device_osdep meta;
};

struct cdi_usb_driver
{
    struct cdi_driver drv;

    void (*get_endpoint_descriptor)(struct cdi_usb_device *dev, int ep, struct cdi_usb_endpoint_descriptor *desc);
    cdi_usb_transmission_result_t (*control_transfer)(struct cdi_usb_device *dev, int ep, const struct cdi_usb_setup_packet *setup, void *data);
    cdi_usb_transmission_result_t (*bulk_transfer)(struct cdi_usb_device *dev, int ep, void *data, size_t size);
};

struct cdi_usb_bus_device_pattern
{
    struct cdi_bus_device_pattern pattern;
    int vendor_id, product_id;
    int class_id, subclass_id, protocol_id;
};

#ifdef __cplusplus
extern "C" {
#endif

void cdi_usb_driver_register(struct cdi_usb_driver *drv);
void cdi_usb_get_endpoint_descriptor(struct cdi_usb_device *dev, int ep, struct cdi_usb_endpoint_descriptor *desc);

cdi_usb_transmission_result_t cdi_usb_control_transfer(struct cdi_usb_device *dev, int ep, const struct cdi_usb_setup_packet *setup, void *data);
cdi_usb_transmission_result_t cdi_usb_bulk_transfer(struct cdi_usb_device *dev, int ep, void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif