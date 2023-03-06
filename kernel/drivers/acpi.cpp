// Copyright (C) 2022-2023  ilobilo

#include <lai/helpers/sci.h>
#include <lai/helpers/pm.h>
#include <lai/drivers/ec.h>
#include <drivers/acpi.hpp>
#include <init/kernel.hpp>
#include <lib/alloc.hpp>
#include <lib/panic.hpp>
#include <lib/time.hpp>
#include <lib/log.hpp>
#include <cpu/cpu.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <lai/host.h>
#include <lai/core.h>
#include <string>

#if defined(__x86_64__)
#include <arch/x86_64/cpu/ioapic.hpp>
#include <arch/x86_64/cpu/idt.hpp>
#include <arch/x86_64/lib/io.hpp>
#endif

namespace acpi
{
    FADTHeader *fadthdr = nullptr;
    MADTHeader *madthdr = nullptr;

    std::vector<SDTHeader*> tables;

    std::vector<MADTLapic> lapics;
    std::vector<MADTIOApic> ioapics;
    std::vector<MADTIso> isos;
    std::vector<MADTIONmi> ionmis;
    std::vector<MADTLNmi> lnmis;
    std::vector<MADTLAPICAO> laddrovers;
    std::vector<MADTLx2APIC> x2apics;

    RSDP *rsdp = nullptr;
    RSDT *rsdt = nullptr;
    bool xsdt = false;

    void *findtable(const char *signature, size_t skip)
    {
        if (!strncmp(signature, "DSDT", 4))
        {
            uintptr_t dsdt_addr = 0;

            if (xsdt == true && vmm::is_canonical(fadthdr->X_Dsdt))
                dsdt_addr = fadthdr->X_Dsdt;
            else
                dsdt_addr = fadthdr->Dsdt;

            return reinterpret_cast<void*>(tohh(dsdt_addr));
        }

        for (const auto &header : tables)
        {
            if (!strncmp(reinterpret_cast<const char*>(header->signature), signature, 4))
            {
                if (skip > 0)
                    skip--;
                else
                    return reinterpret_cast<void*>(header);
            }
        }

        return nullptr;
    }

    void madt_init()
    {
        if (madthdr == nullptr)
            return;

        uintptr_t start = tohh(reinterpret_cast<uintptr_t>(madthdr->entries));
        uintptr_t end = tohh(reinterpret_cast<uintptr_t>(madthdr) + madthdr->sdt.length);

        MADT *madt = reinterpret_cast<MADT*>(start);

        for (uintptr_t entry = start; entry < end; entry += madt->length, madt = reinterpret_cast<MADT*>(entry))
        {
            switch (madt->type)
            {
                case 0:
                    lapics.push_back(*reinterpret_cast<MADTLapic*>(entry));
                    break;
                case 1:
                    ioapics.push_back(*reinterpret_cast<MADTIOApic*>(entry));
                    break;
                case 2:
                    isos.push_back(*reinterpret_cast<MADTIso*>(entry));
                    break;
                case 3:
                    ionmis.push_back(*reinterpret_cast<MADTIONmi*>(entry));
                    break;
                case 4:
                    lnmis.push_back(*reinterpret_cast<MADTLNmi*>(entry));
                    break;
                case 5:
                    laddrovers.push_back(*reinterpret_cast<MADTLAPICAO*>(entry));
                    break;
                case 9:
                    x2apics.push_back(*reinterpret_cast<MADTLx2APIC*>(entry));
                    break;
            }
        }
    }

    void ec_init()
    {
        LAI_CLEANUP_STATE lai_state_t state;
        lai_init_state(&state);

        LAI_CLEANUP_VAR lai_variable_t pnp_id = LAI_VAR_INITIALIZER;
        lai_eisaid(&pnp_id, ACPI_EC_PNP_ID);

        lai_ns_iterator it = LAI_NS_ITERATOR_INITIALIZER;
        lai_nsnode_t *node;

        while ((node = lai_ns_iterate(&it)))
        {
            if (lai_check_device_pnp_id(node, &pnp_id, &state))
                continue;

            auto driver = pmm::alloc<lai_ec_driver*>(div_roundup(sizeof(lai_ec_driver), pmm::page_size));
            lai_init_ec(node, driver);

            lai_ns_child_iterator child_it = LAI_NS_CHILD_ITERATOR_INITIALIZER(node);
            lai_nsnode_t *child_node = nullptr;

            while ((child_node = lai_ns_child_iterate(&child_it)))
                if (lai_ns_get_node_type(child_node) == LAI_NODETYPE_OPREGION)
                    lai_ns_override_opregion(child_node, &lai_ec_opregion_override, driver);
        }
    }

    void poweroff()
    {
        lai_enter_sleep(5);
    }

    void reboot()
    {
        lai_acpi_reset();
    }

    void enable()
    {
        ec_init();

        #if defined(__x86_64__)
        lai_enable_acpi(ioapic::initialised ? 1 : 0);

        uint8_t sci_int = fadthdr->SCI_Interrupt;

        if(madthdr->legacy_pic() == false && ioapic::initialised)
            ioapic::set(sci_int, sci_int + 0x20, ioapic::deliveryMode::FIXED, ioapic::destMode::PHYSICAL, ioapic::ACTIVE_HIGH_LOW | ioapic::EDGE_LEVEL, smp_request.response->bsp_lapic_id);

        idt::handlers[sci_int + 0x20].set([](auto)
        {
            uint16_t event = lai_get_sci_event();
            if (event & ACPI_POWER_BUTTON)
            {
                acpi::poweroff();

                time::msleep(50);
                io::out<uint16_t>(0xB004, 0x2000);
                io::out<uint16_t>(0x604, 0x2000);
                io::out<uint16_t>(0x4004, 0x3400);
            }
        });
        idt::unmask(sci_int);
        #else
        lai_enable_acpi(0);
        #endif
    }

    void init()
    {
        log::infoln("ACPI: Initialising...");

        rsdp = reinterpret_cast<RSDP*>(tohh(rsdp_request.response->address));

        if (rsdp->revision >= 2 && rsdp->xsdtaddr)
        {
            xsdt = true;
            rsdt = reinterpret_cast<RSDT*>(tohh(rsdp->xsdtaddr));
            log::infoln("ACPI: Found XSDT at: 0x{:X}", reinterpret_cast<uintptr_t>(rsdt));
        }
        else
        {
            xsdt = false;
            rsdt = reinterpret_cast<RSDT*>(tohh(rsdp->rsdtaddr));
            log::infoln("ACPI: Found RSDT at: 0x{:X}", reinterpret_cast<uintptr_t>(rsdt));
        }

        size_t entries = (rsdt->header.length - sizeof(SDTHeader)) / (xsdt ? 8 : 4);
        if (entries != 0)
        {
            log::info("ACPI: Found Tables:");
            for (size_t i = 0; i < entries; i++)
            {
                auto header = reinterpret_cast<SDTHeader*>(tohh(xsdt ? rsdt->tablesx[i] : rsdt->tables[i]));
                if (header == nullptr)
                    continue;

                auto checksum = [header]() -> bool
                {
                    uint8_t sum = 0;
                    for (size_t i = 0; i < header->length; i++)
                        sum += reinterpret_cast<uint8_t*>(header)[i];
                    return sum == 0;
                };

                if (checksum())
                {
                    log::print(" {}", std::string_view(reinterpret_cast<const char *>(header->signature), 4));
                    tables.push_back(header);
                }
            }
            log::println();
        }

        madthdr = reinterpret_cast<MADTHeader*>(findtable("APIC", 0));
        fadthdr = reinterpret_cast<FADTHeader*>(findtable("FACP", 0));

        madt_init();

        lai_set_acpi_revision(rsdp->revision);
        lai_create_namespace();
    }
} // namespace acpi

void *laihost_scan(const char *signature, size_t index)
{
    return acpi::findtable(signature, index);
}