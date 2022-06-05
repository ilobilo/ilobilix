// Copyright (C) 2022  ilobilo

#include <drivers/timers/pit/pit.hpp>
#include <cpu/idt/idt.hpp>
#include <lib/timer.hpp>
#include <cdi/misc.h>

extern "C"
{
    static volatile uint8_t triggered = 0;
    void cdi_register_irq(uint8_t irq, void (*handler)(cdi_device*), cdi_device *device)
    {
        auto _handler = [=](cpu::registers_t*)
        {
            triggered |= (1 << irq);
            handler(device);
        };
        idt::handlers[irq].set(_handler);
    }

    int cdi_reset_wait_irq(uint8_t irq)
    {
        triggered &= ~(1 << irq);
        return 0;
    }

    int cdi_wait_irq(uint8_t irq, uint32_t timeout)
    {
        uint64_t start = cdi_elapsed_ms();
        uint64_t end = start + timeout;
        while (!(triggered & (1 << irq)))
        {
            if (cdi_elapsed_ms() >= end) return -1;
            timer::msleep(1);
        }
        return 0;
    }

    int cdi_ioports_alloc(uint16_t start, uint16_t count) { return 0; }
    int cdi_ioports_free(uint16_t start, uint16_t count) { return 0; }

    void cdi_sleep_ms(uint32_t ms)
    {
        timer::msleep(ms);
    }

    uint64_t cdi_elapsed_ms()
    {
        return (timers::pit::get_tick() * 100) / timers::pit::frequency;
    }
} // extern "C"