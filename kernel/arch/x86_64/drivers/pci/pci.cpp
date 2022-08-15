// Copyright (C) 2022  ilobilo

#include <arch/x86_64/drivers/pci/pci_legacy.hpp>
#include <arch/x86_64/drivers/pci/pci_acpi.hpp>
#include <drivers/pci/pci.hpp>

namespace arch
{
    static void addlegacyio()
    {
        auto io = new pci::legacy_configio;
        for (size_t i = 0; i < 256; i++)
            pci::addconfigio(0, i, io);
    }

    void pci_init()
    {
        if (pci::add_acpi_configio() == false)
            addlegacyio();

        pci::add_acpi_rootbusses();
    }
} // namespace arch