// Copyright (C) 2022  ilobilo

#include <cdi/misc.h>

extern "C"
{
    void cdi_register_irq(uint8_t irq, void (*handler)(cdi_device*), cdi_device *device);
    int cdi_reset_wait_irq(uint8_t irq);
    int cdi_wait_irq(uint8_t irq, uint32_t timeout);
    int cdi_ioports_alloc(uint16_t start, uint16_t count);
    int cdi_ioports_free(uint16_t start, uint16_t count);
    void cdi_sleep_ms(uint32_t ms);
    uint64_t cdi_elapsed_ms();
} // extern "C"