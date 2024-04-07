// Copyright (C) 2022-2024  ilobilo

#include <drivers/pci/pci_acpi.hpp>
#include <drivers/acpi.hpp>

namespace pci
{
    void arch_init()
    {
        pci::acpi::init_ios();
        ::acpi::enable();
        pci::acpi::init_rbs();
    }
} // namespace pci