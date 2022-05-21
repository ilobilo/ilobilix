// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__x86_64__)

#include <cstddef>
#include <cstdint>

namespace arch::x86_64::apic
{
    enum events
    {
        ACPI_TIMER = 0x0001,
        ACPI_BUSMASTER = 0x0010,
        ACPI_GLOBAL = 0x0020,
        ACPI_POWER_BUTTON = 0x0100,
        ACPI_SLEEP_BUTTON = 0x0200,
        ACPI_RTC_ALARM = 0x0400,
        ACPI_PCIE_WAKE = 0x4000,
        ACPI_WAKE = 0x8000
    };

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
} // namespace arch::x86_64::apic

#endif