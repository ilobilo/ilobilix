/*
 * Copyright (c) 2008 Mathias Gottschlag
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_BIOS_H_
#define _CDI_BIOS_H_

#include <stdint.h>

#include <cdi/lists.h>

struct cdi_bios_registers
{
    uint16_t ax;
    uint16_t bx;
    uint16_t cx;
    uint16_t dx;
    uint16_t si;
    uint16_t di;
    uint16_t ds;
    uint16_t es;
};

struct cdi_bios_memory
{
    uintptr_t dest;
    void *src;
    uint16_t size;
};

#ifdef __cplusplus
extern "C" {
#endif

int cdi_bios_int10(struct cdi_bios_registers *registers, cdi_list_t memory);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

