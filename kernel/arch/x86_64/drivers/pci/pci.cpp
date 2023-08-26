// Copyright (C) 2022-2023  ilobilo

#include <arch/x86_64/drivers/pci/pci_legacy.hpp>
#include <drivers/pci/pci_acpi.hpp>

namespace pci
{
    void arch_init()
    {
        if (pci::acpi::init_ios() == false)
            pci::legacy::init_ios();

        // We run this here because it might need pci to work
        lai_create_namespace();

        if (pci::acpi::init_rbs() == false)
            pci::legacy::init_rbs();
    }
} // namespace pci