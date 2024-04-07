// Copyright (C) 2022-2024  ilobilo

#include <arch/x86_64/drivers/pci/pci_legacy.hpp>
#include <drivers/pci/pci_acpi.hpp>
#include <drivers/acpi.hpp>

namespace pci
{
    void arch_init()
    {
        if (pci::acpi::init_ios() == false)
            pci::legacy::init_ios();

        ::acpi::enable();

        if (pci::acpi::init_rbs() == false)
            pci::legacy::init_rbs();
    }
} // namespace pci