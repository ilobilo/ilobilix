// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <uacpi/tables.h>
#include <uacpi/acpi.h>

#include <vector>

namespace acpi
{
    namespace madt
    {
        extern acpi_madt *hdr;

        extern std::vector<acpi_madt_ioapic> ioapics;
        extern std::vector<acpi_madt_interrupt_source_override> isos;

        inline bool legacy_pic()
        {
            return hdr && hdr->flags & 0x01;
        }

        void init();
    } // namespace madt

    acpi_fadt *get_fadt();

    void poweroff();
    void reboot();

    void enable();
    void init();
} // namespace acpi