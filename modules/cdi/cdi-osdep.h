/*
 * Copyright (c) 2009 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_OSDEP_H_
#define _CDI_OSDEP_H_

#define DRIVER_SECTION ".drivers"

#define cdi_glue(x, y) x ## y

#ifdef __cplusplus
#define cdi_declare_driver(drv, counter) extern "C" const void *__attribute__((section(DRIVER_SECTION), used)) cdi_glue(__cdi_driver_, counter) = reinterpret_cast<const void*>(&drv);
#else
#define cdi_declare_driver(drv, counter) static const void *__attribute__((section(DRIVER_SECTION), used)) cdi_glue(__cdi_driver_, counter) = &drv;
#endif

#define CDI_DRIVER(name, drv, deps...) cdi_declare_driver(drv, __COUNTER__)

typedef struct
{
    uint16_t seg;
} cdi_pci_device_osdep;

typedef struct
{
} cdi_usb_device_osdep;

typedef struct
{
} cdi_dma_osdep;

typedef struct
{
    bool malloced;
} cdi_mem_osdep;

typedef struct
{
} cdi_fs_osdep;

#endif