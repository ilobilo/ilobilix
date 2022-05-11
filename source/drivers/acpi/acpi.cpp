// Copyright (C) 2022  ilobilo

#include <drivers/acpi/acpi.hpp>
#include <lai/helpers/pm.h>
#include <mm/vmm/vmm.hpp>
#include <lib/string.hpp>
#include <lib/timer.hpp>
#include <lib/alloc.hpp>
#include <lib/panic.hpp>
#include <lib/log.hpp>
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


void laihost_log(int level, const char *msg)
{
    switch (level)
    {
        case LAI_DEBUG_LOG:
            log::info("%s", msg);
            break;
        case LAI_WARN_LOG:
            log::warn("%s", msg);
            break;
    }
}

[[gnu::noreturn]] void laihost_panic(const char *msg)
{
    log::error("%s", msg);
    while (true)
    {
        #if defined(__x86_64__) || defined(_M_X64)
        asm volatile ("cli; hlt");
        #endif
    }
}

void *laihost_malloc(size_t size)
{
    return malloc(size);
}

void *laihost_realloc(void *ptr, size_t size, size_t oldsize)
{
    return realloc(ptr, size);
}

void laihost_free(void *ptr, size_t oldsize)
{
    free(ptr);
}

void *laihost_map(size_t address, size_t count)
{
    for (size_t i = 0; i < count; i += 0x1000)
    {
        mm::vmm::kernel_pagemap->mapMem(address + hhdm_offset, address);
    }
    return reinterpret_cast<void*>(address + hhdm_offset);
}

void laihost_unmap(void *address, size_t count)
{
    for (size_t i = 0; i < count; i += 0x1000)
    {
        mm::vmm::kernel_pagemap->unmapMem(reinterpret_cast<uint64_t>(address) + i);
    }
}

void *laihost_scan(const char *signature, size_t index)
{
    return acpi::findtable(signature, index);
}

void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint8_t val)
{
    // pci::writeb(seg, bus, slot, fun, offset, val);
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint16_t val)
{
    // pci::writew(seg, bus, slot, fun, offset, val);
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint32_t val)
{
    // pci::writel(seg, bus, slot, fun, offset, val);
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    // pci::readb(seg, bus, slot, fun, offset);
    return 0;
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    // return pci::readw(seg, bus, slot, fun, offset);
    return 0;
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    // return pci::readl(seg, bus, slot, fun, offset);
    return 0;
}