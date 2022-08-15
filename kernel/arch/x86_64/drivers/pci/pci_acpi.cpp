// Copyright (C) 2022  ilobilo

#include <arch/x86_64/drivers/pci/pci_legacy.hpp>
#include <drivers/pci/pci_ecam.hpp>
#include <drivers/pci/pci.hpp>
#include <lai/helpers/pci.h>
#include <drivers/acpi.hpp>
#include <lib/log.hpp>

namespace pci
{
    static uint64_t eval_aml_method(lai_state_t *state, lai_nsnode_t *node, const char *name)
    {
        LAI_CLEANUP_VAR lai_variable_t var = LAI_VAR_INITIALIZER;
        uint64_t ret = 0;
        lai_nsnode_t *handle = lai_resolve_path(node, name);

        if (handle == nullptr)
        {
            log::warn("PCI: Root bus doesn't have %s, assuming 0", name);
            return 0;
        }

        if (auto e = lai_eval(&var, handle, state); e != LAI_ERROR_NONE)
        {
            log::warn("PCI: Couldn't evaluate Root Bus %s, assuming 0", name);
            return 0;
        }

        if (auto e = lai_obj_get_integer(&var, &ret); e != LAI_ERROR_NONE)
        {
            log::warn("PCI: Root Bus %s evaluation didn't result in an integer, assuming 0", name);
            return 0;
        }

        return ret;
    }

    bool add_acpi_configio()
    {
        auto mcfg = acpi::findtable<acpi::MCFGHeader>("MCFG", 0);
        if (mcfg == nullptr)
        {
            log::warn("PCI: MCFG table not found!");
            return false;
        }
        else if (mcfg->header.length < sizeof(acpi::MCFGHeader) + sizeof(acpi::MCFGEntry))
        {
            log::error("PCI: No entries found in MCFG table!");
            return false;
        }

        // TODO: Can there be multiple mcfg tables?
        for (size_t i = 0; mcfg != nullptr; i++, mcfg = acpi::findtable<acpi::MCFGHeader>("MCFG", i))
        {
            size_t entries = ((mcfg->header.length) - sizeof(acpi::MCFGHeader)) / sizeof(acpi::MCFGEntry);
            for (size_t i = 0; i < entries; i++)
            {
                auto &entry = mcfg->entries[i];
                auto io = new ecam_configio(entry.baseaddr, entry.segment, entry.startbus, entry.endbus);

                for (size_t b = entry.startbus; b <= entry.endbus; b++)
                    addconfigio(entry.segment, b, io);
            }
        }

        return true;
    }

    void add_acpi_rootbusses()
    {
        LAI_CLEANUP_STATE lai_state_t state;
        lai_init_state(&state);

        LAI_CLEANUP_VAR lai_variable_t pci_pnp_id = { };
        LAI_CLEANUP_VAR lai_variable_t pcie_pnp_id = { };
        lai_eisaid(&pci_pnp_id, ACPI_PCI_ROOT_BUS_PNP_ID);
        lai_eisaid(&pcie_pnp_id, ACPI_PCIE_ROOT_BUS_PNP_ID);

        lai_nsnode_t *sb_handle = lai_resolve_path(nullptr, "\\_SB_");
        LAI_ENSURE(sb_handle);

        lai_ns_child_iterator it = LAI_NS_CHILD_ITERATOR_INITIALIZER(sb_handle);
        lai_nsnode_t *handle = nullptr;

        while ((handle = lai_ns_child_iterate(&it)))
        {
            if (lai_check_device_pnp_id(handle, &pci_pnp_id, &state) && lai_check_device_pnp_id(handle, &pcie_pnp_id, &state))
                continue;

            auto seg = eval_aml_method(&state, handle, "_SEG");
            auto bbn = eval_aml_method(&state, handle, "_BBN");

            addrootbus(new bus_t(nullptr, getconfigio(seg, bbn), seg, bbn));
        }
    }
} // namespace pci