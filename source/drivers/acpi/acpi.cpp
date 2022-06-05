// Copyright (C) 2022  ilobilo

#include <drivers/acpi/acpi.hpp>
#include <lai/helpers/sci.h>
#include <lai/helpers/pm.h>
#include <mm/vmm/vmm.hpp>
#include <lib/string.hpp>
#include <lib/timer.hpp>
#include <lib/alloc.hpp>
#include <lib/panic.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>
#include <lai/host.h>
#include <lai/core.h>
#include <main.hpp>

namespace acpi
{
    FADTHeader *fadthdr = nullptr;
    HPETHeader *hpethdr = nullptr;
    MCFGHeader *mcfghdr = nullptr;
    MADTHeader *madthdr = nullptr;
    bool madt = false;

    vector<SDTHeader*> tables;
    vector<MADTLapic*> lapics;
    vector<MADTIOApic*> ioapics;
    vector<MADTIso*> isos;
    vector<MADTNmi*> nmis;

    uintptr_t lapic_addr = 0;

    RSDP *rsdp = nullptr;
    RSDT *rsdt = nullptr;
    bool xsdt = false;

    void *findtable(const char *signature, size_t skip)
    {
        if (skip < 0) skip = 0;

        if (!strncmp(signature, "DSDT", 4))
        {
            uint64_t dsdt_addr = 0;

            if (((fadthdr->X_Dsdt <= 0x00007FFFFFFFFFFF) || ((fadthdr->X_Dsdt >= 0xFFFF800000000000) && (fadthdr->X_Dsdt <= 0xFFFFFFFFFFFFFFFF))) && xsdt) dsdt_addr = acpi::fadthdr->X_Dsdt;
            else dsdt_addr = fadthdr->Dsdt;

            return reinterpret_cast<void*>(dsdt_addr);
        }

        for (const auto header : tables)
        {
            if (!strncmp(reinterpret_cast<const char*>(header->signature), signature, 4))
            {
                if (skip > 0) skip--;
                else return reinterpret_cast<void*>(header);
            }
        }

        return nullptr;
    }

    void madt_init()
    {
        lapic_addr = madthdr->local_controller_addr;

        for (uint8_t *madt_ptr = reinterpret_cast<uint8_t*>(madthdr->entries_begin); reinterpret_cast<uintptr_t>(madt_ptr) < reinterpret_cast<uintptr_t>(madthdr) + madthdr->sdt.length; madt_ptr += *(madt_ptr + 1))
        {
            switch (*(madt_ptr))
            {
                case 0:
                    log::info("ACPI: Found Local APIC %ld", lapics.size());
                    lapics.push_back(reinterpret_cast<MADTLapic*>(madt_ptr));
                    break;
                case 1:
                    log::info("ACPI: Found I/O APIC %ld", ioapics.size());
                    ioapics.push_back(reinterpret_cast<MADTIOApic*>(madt_ptr));
                    break;
                case 2:
                    log::info("ACPI: Found ISO %ld", isos.size());
                    isos.push_back(reinterpret_cast<MADTIso*>(madt_ptr));
                    break;
                case 4:
                    log::info("ACPI: Found NMI %ld", nmis.size());
                    nmis.push_back(reinterpret_cast<MADTNmi*>(madt_ptr));
                    break;
                case 5:
                    lapic_addr = *reinterpret_cast<uint64_t*>(madt_ptr + 4);
                    break;
            }
        }
    }

    void shutdown()
    {
        lai_enter_sleep(5);
    }

    void reboot()
    {
        lai_acpi_reset();
        // ps2::reboot();
    }

    void enable()
    {
        // TODO: Temporary
        // lai_enable_acpi(apic::initialised ? 1  : 0);
        // temp {
            outb(acpi::fadthdr->SMI_CommandPort, acpi::fadthdr->AcpiEnable);
            while ((inw(acpi::fadthdr->PM1aControlBlock) & 0x01) == 0);
            lai_set_sci_event(ACPI_POWER_BUTTON | ACPI_SLEEP_BUTTON | ACPI_WAKE);
            lai_get_sci_event();
        // } temp
    }

    void init()
    {
        log::info("Initialising ACPI...");

        rsdp = reinterpret_cast<RSDP*>(rsdp_request.response->address);

        if (rsdp->revision >= 2 && rsdp->xsdtaddr)
        {
            xsdt = true;
            rsdt = reinterpret_cast<RSDT*>(rsdp->xsdtaddr);
            log::info("Found XSDT at: 0x%X", rsdp->xsdtaddr);
        }
        else
        {
            xsdt = false;
            rsdt = reinterpret_cast<RSDT*>(rsdp->rsdtaddr);
            log::info("Found RSDT at: 0x%X", rsdp->rsdtaddr);
        }

        size_t entries = (rsdt->header.length - sizeof(SDTHeader)) / (xsdt ? 8 : 4);
        for(size_t i = 0; i < entries; i++)
        {
            auto header = reinterpret_cast<SDTHeader*>(xsdt ? rsdt->tablesx[i] : rsdt->tables[i]);
            if (header == nullptr) continue;

            auto checksum = [header]() -> bool
            {
                uint8_t sum = 0;
                for (size_t i = 0; i < header->length; i++) sum += reinterpret_cast<uint8_t*>(header)[i];
                return sum == 0;
            };

            if(checksum())
            {
                log::info("ACPI: Found Table %.4s", header->signature);
                tables.push_back(header);
            }
        }

        mcfghdr = reinterpret_cast<MCFGHeader*>(findtable("MCFG", 0));
        madthdr = reinterpret_cast<MADTHeader*>(findtable("APIC", 0));
        if (madthdr != nullptr) madt = true;
        fadthdr = reinterpret_cast<FADTHeader*>(findtable("FACP", 0));
        hpethdr = reinterpret_cast<HPETHeader*>(findtable("HPET", 0));

        if (madt == true) madt_init();

        lai_set_acpi_revision(rsdp->revision);
        lai_create_namespace();
    }
} // namespace acpi

void *laihost_scan(const char *signature, size_t index)
{
    return acpi::findtable(signature, index);
}