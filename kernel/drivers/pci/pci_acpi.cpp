// Copyright (C) 2022-2024  ilobilo

#include <drivers/pci/pci_ecam.hpp>
#include <drivers/acpi.hpp>

#include <lib/panic.hpp>
#include <lib/log.hpp>

#include <uacpi/utilities.h>
#include <uacpi/resources.h>
#include <uacpi/uacpi.h>

namespace pci::acpi
{
    struct router : irq_router
    {
        private:
        uacpi_namespace_node *node;

        public:
        router(irq_router *parent, bus_t *bus, uacpi_namespace_node *node) : irq_router(parent, bus), node(node)
        {
            if (this->node == nullptr)
            {
                for (size_t i = 0; i < 4; i++)
                    this->bridgeirqs[i] = this->parent->resolve(this->mybus->bridge->dev, i + 1);

                this->mod = irq_router::model::expansion;
                return;
            }

            uacpi_pci_routing_table *pci_routes;
            auto ret = uacpi_get_pci_routing_table(this->node, &pci_routes);
            if (ret == UACPI_STATUS_NOT_FOUND)
            {
                if (this->parent != nullptr)
                {
                    log::warnln("PCI: No '_PRT' for bus. assuming expansion bridge routing");
                    for (size_t i = 0; i < 4; i++)
                        this->bridgeirqs[i] = this->parent->resolve(this->mybus->bridge->dev, i + 1);

                    this->mod = irq_router::model::expansion;
                }
                else log::errorln("PCI: No '_PRT' for bus. giving up IRQ routing for this bus");
                return;
            }
            else if (ret != UACPI_STATUS_OK)
            {
                log::errorln("PCI: Failed to evaluate '_PRT': {}", uacpi_status_to_string(ret));
                return;
            }

            for (size_t i = 0; i < pci_routes->num_entries; i++)
            {
                auto entry = &pci_routes->entries[i];

                auto triggering = flags::level;
                auto polarity = flags::low;

                auto gsi = entry->index;

                int32_t slot = (entry->address >> 16) & 0xFFFF;
                int32_t func = entry->address & 0xFFFF;
                if (func == 0xFFFF)
                    func = -1;

                assert((entry->address & 0xFFFF) == 0xFFFF, "PCI: TODO: support routing of individual functions");

                if (entry->source)
                {
                    assert(entry->index == 0);

                    uacpi_resources *resources;
                    ret = uacpi_get_current_resources(entry->source, &resources);
                    assert(ret == UACPI_STATUS_OK);

                    switch (resources->entries[0].type)
                    {
                        case UACPI_RESOURCE_TYPE_IRQ:
                        {
                            auto *irq = &resources->entries[0].irq;
                            assert(irq->num_irqs >= 1);
                            gsi = irq->irqs[0];

                            if (irq->triggering == UACPI_TRIGGERING_EDGE)
                                triggering = flags::edge;
                            if (irq->polarity == UACPI_POLARITY_ACTIVE_HIGH)
                                polarity = flags::high;

                            break;
                        }
                        case UACPI_RESOURCE_TYPE_EXTENDED_IRQ:
                        {
                            auto *irq = &resources->entries[0].extended_irq;
                            assert(irq->num_irqs >= 1);
                            gsi = irq->irqs[0];

                            if (irq->triggering == UACPI_TRIGGERING_EDGE)
                                triggering = flags::edge;
                            if (irq->polarity == UACPI_POLARITY_ACTIVE_HIGH)
                                polarity = flags::high;

                            break;
                        }
                        default:
                            PANIC("PCI: Invalid link '_CRS' type");
                    }

                    uacpi_free_resources(resources);
                }

                this->table.emplace_back(
                    gsi, slot, func, entry->pin + 1,
                    triggering | polarity
                );
            }

            uacpi_free_pci_routing_table(pci_routes);
            this->mod = irq_router::model::root;
        }

        router *downstream(bus_t *bus) override
        {
            uacpi_namespace_node *dhandle = nullptr;
            if (this->node != nullptr)
            {
                struct devctx
                {
                    uint64_t addr;
                    uacpi_namespace_node *out;
                } ctx {
                    .addr = static_cast<uint64_t>((bus->bridge->dev << 16) | bus->bridge->func),
                    .out = nullptr
                };

                uacpi_namespace_for_each_node_depth_first(this->node,
                    [](uacpi_handle opaque, uacpi_namespace_node *node) -> uacpi_ns_iteration_decision
                    {
                        auto *ctx = reinterpret_cast<devctx *>(opaque);
                        uint64_t addr = 0;

                        auto *obj = uacpi_namespace_node_get_object(node);
                        if (obj == nullptr || obj->type != UACPI_OBJECT_DEVICE)
                            return UACPI_NS_ITERATION_DECISION_CONTINUE;

                        auto ret = uacpi_eval_integer(node, "_ADR", UACPI_NULL, &addr);
                        if (ret != UACPI_STATUS_OK && ret != UACPI_STATUS_NOT_FOUND)
                            return UACPI_NS_ITERATION_DECISION_CONTINUE;

                        if (addr == ctx->addr)
                        {
                            ctx->out = node;
                            return UACPI_NS_ITERATION_DECISION_BREAK;
                        }

                        return UACPI_NS_ITERATION_DECISION_CONTINUE;
                    }, &ctx
                );

                dhandle = ctx.out;
            }
            return new router(this, bus, dhandle);
        }
    };

    bool init_ios()
    {
        using namespace ::acpi;

        uacpi_table mcfgtable;
        auto ret = uacpi_table_find_by_signature("MCFG", &mcfgtable);
        if (ret == UACPI_STATUS_NOT_FOUND)
        {
            log::warnln("PCI: MCFG table not found");
            return false;
        }

        auto *mcfg = reinterpret_cast<acpi_mcfg *>(mcfgtable.virt_addr);

        if (mcfg->hdr.length < sizeof(acpi_mcfg) + sizeof(acpi_mcfg_allocation))
        {
            log::warnln("PCI: MCFG table has no entries");
            return false;
        }

        size_t entries = ((mcfg->hdr.length) - sizeof(acpi_mcfg)) / sizeof(acpi_mcfg_allocation);
        for (size_t i = 0; i < entries; i++)
        {
            auto &entry = mcfg->entries[i];
            auto io = new ecam::configio(entry.address, entry.segment, entry.start_bus, entry.end_bus);

            for (size_t b = entry.start_bus; b <= entry.end_bus; b++)
                addconfigio(entry.segment, b, io);
        }

        return true;
    }

    bool found = false;
    bool init_rbs()
    {
        static const char *root_ids[] =
        {
            "PNP0A03",
            "PNP0A08",
            nullptr,
        };

        uacpi_find_devices_at(
            uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SB),
            root_ids, [](void *, uacpi_namespace_node *node) -> uacpi_ns_iteration_decision
            {
                found = true;

                uint64_t seg = 0, bus = 0;

                uacpi_eval_integer(node, "_SEG", nullptr, &seg);
                uacpi_eval_integer(node, "_BBN", nullptr, &bus);

                auto rbus = new bus_t(nullptr, nullptr, getconfigio(seg, bus), seg, bus);
                rbus->router = new router(nullptr, rbus, node);
                addrootbus(rbus);

                return UACPI_NS_ITERATION_DECISION_CONTINUE;
            }, nullptr
        );

        return found;
    }
} // namespace pci::acpi