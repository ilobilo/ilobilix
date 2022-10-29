// Copyright (C) 2022  ilobilo

#include <drivers/pci/pci_acpi.hpp>

namespace pci
{
    void arch_init()
    {
        pci::acpi::init_ios();
    }
} // namespace pci