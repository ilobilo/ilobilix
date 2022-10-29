// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

namespace lapic
{
    enum class timerModes : uint8_t
    {
        ONESHOT = 0,
        PERIODIC = 1,
        TSC_DEADLINE = 2
    };

    class lapic
    {
        private:
        uint32_t ticks_per_ms = 0;
        uintptr_t mmio_base = 0;
        bool x2apic = false;

        bool x2apic_check();
        uint32_t reg2x2apic(uint32_t reg)
        {
            return (reg >> 4) + 0x800;
        }

        uint32_t read(uint32_t reg);
        void write(uint32_t reg, uint64_t value);

        void timer_calibrate();

        public:
        uint32_t id = 0;

        lapic() : ticks_per_ms(0), mmio_base(0), x2apic(false), id(0) { }

        void init();
        void ipi(uint32_t flags, uint32_t id);
        void eoi();

        void timer(uint8_t vector, uint64_t ms, timerModes mode);
    };
} // namespace lapic
