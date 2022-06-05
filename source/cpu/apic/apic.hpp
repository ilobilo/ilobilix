// Copyright (C) 2022  ilobilo

#pragma once

#include <cstddef>
#include <cstdint>

namespace apic
{
    extern bool initialised;

    namespace lapic
    {
        uint32_t read(uint32_t reg);
        void write(uint32_t reg, uint32_t value);

        namespace timer
        {
            void oneshot(uint8_t vector, uint64_t ms = 1);
            void periodic(uint8_t vector, uint64_t ms = 1);
            void init();
        } // namespace timer

        void init(uint8_t processor_id);
    } // namespace lapic

    namespace ioapic
    {
        uint32_t read(uintptr_t address, size_t reg);
        void write(uintptr_t address, size_t reg, uint32_t data);

        void redirect_gsi(uint32_t gsi, uint8_t vec, uint16_t flags);
        void redirect_irq(uint32_t irq, uint8_t vect);
    } // namespace ioapic

    void send_ipi(uint32_t lapic_id, uint32_t flags);
    void eoi();

    void init();
} // namespace apic