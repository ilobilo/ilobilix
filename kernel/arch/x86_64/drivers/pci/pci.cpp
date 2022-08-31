// Copyright (C) 2022  ilobilo

#include <arch/x86_64/drivers/pci/pci_legacy.hpp>
#include <arch/x86_64/drivers/pci/pci_acpi.hpp>
#include <drivers/pci/pci.hpp>

namespace pci
{
    static void add_legacyio()
    {
        auto io = new legacy_configio;
        for (size_t i = 0; i < 256; i++)
            addconfigio(0, i, io);
    }

    static void add_legacy_rootbusses()
    {
        auto io = getconfigio(0, 0);
        if (io->read<uint8_t>(0, 0, 0, 0, PCI_HEADER_TYPE) & 0x80)
        {
            for (size_t i = 0; i < 8; i++)
            {
                if (io->read<uint16_t>(0, 0, 0, i, PCI_VENDOR_ID) == 0xFFFF)
                    continue;

                addrootbus(new bus_t(nullptr, getconfigio(0, i), 0, i));
            }
        }
        else addrootbus(new bus_t(nullptr, getconfigio(0, 0), 0, 0));
    }

    void arch_init()
    {
        if (add_acpi_configio() == false)
            add_legacyio();

        if (add_acpi_rootbusses() == false)
            add_legacy_rootbusses();
    }
} // namespace pci