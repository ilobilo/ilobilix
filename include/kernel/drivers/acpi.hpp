// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <uacpi/tables.h>
#include <uacpi/acpi.h>

#include <cstdint>
#include <vector>

namespace acpi
{
    struct [[gnu::packed]] rsdt_t
    {
        acpi_sdt_hdr header;
        union
        {
            uint32_t tables[];
            uint64_t tablesx[];
        };
    };

    namespace mcfg
    {
        struct [[gnu::packed]] entry
        {
            uint64_t baseaddr;
            uint16_t segment;
            uint8_t startbus;
            uint8_t endbus;
            uint32_t reserved;
        };

        struct [[gnu::packed]] header
        {
            acpi_sdt_hdr header;
            uint64_t reserved;
            entry entries[];
        };
    } // namespace mcfg

    namespace madt
    {
        struct [[gnu::packed]] header
        {
            acpi_sdt_hdr sdt;
            uint32_t lapic_addr;
            uint32_t flags;
            char entries[];

            bool legacy_pic()
            {
                return this->flags & 0x01;
            }
        };

        struct [[gnu::packed]] madt_t
        {
            uint8_t type;
            uint8_t length;
        };

        // struct [[gnu::packed]] lapic
        // {
        //     madt_t madt_header;
        //     uint8_t processor_id;
        //     uint8_t apic_id;
        //     uint32_t flags;
        // };

        struct [[gnu::packed]] ioapic
        {
            madt_t madt_header;
            uint8_t apic_id;
            uint8_t reserved;
            uint32_t addr;
            uint32_t gsib;
        };

        struct [[gnu::packed]] iso
        {
            madt_t madt_header;
            uint8_t bus_source;
            uint8_t irq_source;
            uint32_t gsi;
            uint16_t flags;
        };

        // struct [[gnu::packed]] ionmi
        // {
        //     madt_t madt_header;
        //     uint8_t source;
        //     uint8_t reserved;
        //     uint16_t flags;
        //     uint32_t gsi;
        // };

        // struct [[gnu::packed]] lnmi
        // {
        //     madt_t madt_header;
        //     uint8_t processor;
        //     uint16_t flags;
        //     uint8_t lint;
        // };

        // struct [[gnu::packed]] lapicao
        // {
        //     madt_t madt_header;
        //     uint16_t reserved;
        //     uint64_t lapic_addr;
        // };

        // struct [[gnu::packed]] x2apic
        // {
        //     madt_t madt_header;
        //     uint16_t reserved;
        //     uint32_t x2apicid;
        //     uint32_t flags;
        //     uint32_t acpi_id;
        // };

        extern header *hdr;

        // extern std::vector<lapic> lapics;
        extern std::vector<ioapic> ioapics;
        extern std::vector<iso> isos;
        // extern std::vector<ionmi> ionmis;
        // extern std::vector<lnmi> lnmis;
        // extern std::vector<lapicao> laddrovers;
        // extern std::vector<x2apic> x2apics;

        void init();
    } // namespace madt

    struct [[gnu::packed]] genericaddr
    {
        uint8_t aspace;
        uint8_t width;
        uint8_t offset;
        uint8_t size;
        uint64_t address;
    };

    acpi_fadt *get_fadt();

    void poweroff();
    void reboot();

    void enable();
    void init();

    inline uacpi_object_name signature(const char str[5])
    {
        return { { str[0], str[1], str[2], str[3] } };
    }
} // namespace acpi