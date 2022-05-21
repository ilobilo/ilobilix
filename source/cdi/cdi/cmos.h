/*
 * Copyright (c) 2008 Matthew Iselin
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_CMOS_H_
#define _CDI_CMOS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t cdi_cmos_read(uint8_t index);
void cdi_cmos_write(uint8_t index, uint8_t value);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
