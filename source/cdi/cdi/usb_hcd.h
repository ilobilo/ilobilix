/*
 * Copyright (c) 2015 Max Reitz
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_USB_HCD_H_
#define _CDI_USB_HCD_H_

#include <cdi.h>
#include <cdi/usb.h>

typedef void *cdi_usb_hc_transaction_t;

typedef enum
{
    CDI_USB_IN,
    CDI_USB_OUT,
    CDI_USB_SETUP,
} cdi_usb_transfer_token_t;

struct cdi_usb_hc
{
    struct cdi_device dev;
    struct cdi_usb_hub rh;
};

struct cdi_usb_hc_bus
{
    struct cdi_bus_data bus_data;
    struct cdi_usb_hc* hc;
};

struct cdi_usb_hc_ep_info
{
    uint8_t dev;
    uint8_t ep;
    cdi_usb_endpoint_type_t ep_type;
    cdi_usb_speed_t speed;
    size_t mps;
    uint8_t tt_addr, tt_port;
};

struct cdi_usb_hc_transmission
{
    cdi_usb_hc_transaction_t ta;
    cdi_usb_transfer_token_t token;
    void *buffer;
    size_t size;
    unsigned toggle;
    cdi_usb_transmission_result_t* result;
};

struct cdi_usb_hcd
{
    struct cdi_driver drv;
    struct cdi_usb_hub_driver rh_drv;

    cdi_usb_hc_transaction_t (*create_transaction)(struct cdi_usb_hc *hc, const struct cdi_usb_hc_ep_info *target);
    void (*enqueue)(struct cdi_usb_hc *hc, const struct cdi_usb_hc_transmission *trans);
    void (*start_transaction)(struct cdi_usb_hc *hc, cdi_usb_hc_transaction_t ta);
    void (*wait_transaction)(struct cdi_usb_hc *hc, cdi_usb_hc_transaction_t ta);
    void (*destroy_transaction)(struct cdi_usb_hc *hc, cdi_usb_hc_transaction_t ta);
};

#endif