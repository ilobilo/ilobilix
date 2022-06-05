// Copyright (C) 2022  ilobilo

#include <cdi/cmos.h>
#include <cdi/io.h>

extern "C"
{
    uint8_t cdi_cmos_read(uint8_t index)
    {
        cdi_outb(0x70, index);
        return cdi_inb(0x71);
    }

    void cdi_cmos_write(uint8_t index, uint8_t value)
    {
        cdi_outb(0x70, index);
        cdi_outb(0x71, value);
    }
} // extern "C"