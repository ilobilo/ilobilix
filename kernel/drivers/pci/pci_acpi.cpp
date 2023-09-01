// Copyright (C) 2022-2023  ilobilo

#include <drivers/pci/pci_ecam.hpp>
#include <lai/helpers/pci.h>
#include <drivers/acpi.hpp>
#include <lib/log.hpp>

namespace pci::acpi
{
    struct router : irq_router
    {
        private:
        lai_nsnode_t *ahandle;

        public:
        router(irq_router *parent, bus_t *bus, lai_nsnode_t *handle) : irq_router(parent, bus), ahandle(handle)
        {
            LAI_CLEANUP_STATE lai_state_t state;
            lai_init_state(&state);

            if (this->ahandle == nullptr)
            {
                for (size_t i = 0; i < 4; i++)
                    this->bridgeirqs[i] = this->parent->resolve(this->mybus->bridge->dev, i + 1);

                this->mod = irq_router::model::expansion;
                return;
            }

            lai_nsnode_t *prthandle = lai_resolve_path(this->ahandle, "_PRT");
            if (prthandle == nullptr)
            {
                if (this->parent != nullptr)
                {
                    log::warnln("PCI: No '_PRT' for bus {}. assuming expansion bridge routing");
                    for (size_t i = 0; i < 4; i++)
                        this->bridgeirqs[i] = this->parent->resolve(this->mybus->bridge->dev, i + 1);
                }
                else
                {
                    log::errorln("PCI: No '_PRT' for bus. giving up IRQ routing for this bus");
                    return;
                }
            }

            LAI_CLEANUP_VAR lai_variable_t prt = LAI_VAR_INITIALIZER;
            if (lai_eval(&prt, prthandle, &state) != LAI_ERROR_NONE)
            {
                log::errorln("PCI: Could not evaluate '_PRT'. giving up IRQ routing for this bus");
                return;
            }

            lai_prt_iterator iter = LAI_PRT_ITERATOR_INITIALIZER(&prt);
            lai_api_error_t err;
            while ((err = lai_pci_parse_prt(&iter)) == LAI_ERROR_NONE)
            {
                this->table.emplace_back(
                    iter.gsi, int32_t(iter.slot), int32_t(iter.function), iter.pin + 1,
                    (iter.level_triggered ? 0 : ACPI_SMALL_IRQ_EDGE_TRIGGERED) |
                    (iter.active_low ? ACPI_SMALL_IRQ_ACTIVE_LOW : 0)
                );
            }
            this->mod = irq_router::model::root;
        }

        router *downstream(bus_t *bus) override
        {
            lai_nsnode_t *dhandle = nullptr;
            if (this->ahandle != nullptr)
            {
                LAI_CLEANUP_STATE lai_state_t state;
                lai_init_state(&state);
                dhandle = lai_pci_find_device(this->ahandle, bus->bridge->dev, bus->bridge->func, &state);
            }
            return new router(this, bus, dhandle);
        }
    };

    static uint64_t eval_aml_method(lai_state_t *state, lai_nsnode_t *node, const char *name)
    {
        LAI_CLEANUP_VAR lai_variable_t var = LAI_VAR_INITIALIZER;
        uint64_t ret = 0;

        lai_nsnode_t *handle = lai_resolve_path(node, name);
        if (handle == nullptr)
        {
            log::warnln("PCI: Root bus doesn't have {}, assuming 0", name);
            return 0;
        }

        if (auto e = lai_eval(&var, handle, state); e != LAI_ERROR_NONE)
        {
            log::warnln("PCI: Couldn't evaluate Root Bus {}, assuming 0", name);
            return 0;
        }

        if (auto e = lai_obj_get_integer(&var, &ret); e != LAI_ERROR_NONE)
        {
            log::warnln("PCI: Root Bus {} evaluation didn't result in an integer, assuming 0", name);
            return 0;
        }

        return ret;
    }

    bool init_ios()
    {
        using namespace ::acpi;

        auto mcfg = findtable<MCFGHeader>("MCFG", 0);
        if (mcfg == nullptr)
        {
            log::warnln("PCI: MCFG table not found!");
            return false;
        }

        bool found = false;
        for (size_t i = 0; mcfg != nullptr; i++, mcfg = findtable<MCFGHeader>("MCFG", i))
        {
            if (mcfg->header.length < sizeof(MCFGHeader) + sizeof(MCFGEntry))
            {
                log::errorln("PCI: No entries found in MCFG table!");
                continue;
            }

            found = true;

            size_t entries = ((mcfg->header.length) - sizeof(MCFGHeader)) / sizeof(MCFGEntry);
            for (size_t i = 0; i < entries; i++)
            {
                auto &entry = mcfg->entries[i];
                auto io = new ecam::configio(entry.baseaddr, entry.segment, entry.startbus, entry.endbus);

                for (size_t b = entry.startbus; b <= entry.endbus; b++)
                    addconfigio(entry.segment, b, io);
            }
        }

        return found;
    }

    bool init_rbs()
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

        bool found = false;
        while ((handle = lai_ns_child_iterate(&it)))
        {
            if (lai_check_device_pnp_id(handle, &pci_pnp_id, &state) && lai_check_device_pnp_id(handle, &pcie_pnp_id, &state))
                continue;

            found = true;

            auto seg = eval_aml_method(&state, handle, "_SEG");
            auto bbn = eval_aml_method(&state, handle, "_BBN");

            auto bus = new bus_t(nullptr, nullptr, getconfigio(seg, bbn), seg, bbn);
            bus->router = new router(nullptr, bus, handle);
            addrootbus(bus);
        }
        return found;
    }
} // namespace pci::acpi