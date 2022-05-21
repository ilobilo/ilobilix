// Copyright (C) 2022  ilobilo

#include <cdi/cmos.h>

extern "C"
{
    uint8_t cdi_cmos_read(uint8_t index);
    void cdi_cmos_write(uint8_t index, uint8_t value);
} // extern "C"