// Copyright (C) 2022-2023  ilobilo

#include <drivers/pci/pci_acpi.hpp>
#include <lai/core.h>

namespace pci
{
    void arch_init()
    {
        pci::acpi::init_ios();

        // We run this here because it might need pci to work
        lai_create_namespace();

        pci::acpi::init_rbs();
    }
} // namespace pci