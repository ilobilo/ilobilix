// Copyright (C) 2022  ilobilo

#include <drivers/acpi/acpi.hpp>
#include <drivers/pci/pci.hpp>
#include <lai/helpers/pci.h>
#include <lib/panic.hpp>
#include <lib/mmio.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>
#include <lai/host.h>

namespace pci
{
    vector<acpi::MCFGEntry> mcfg_entries;
    vector<pcidevice_t*> root_buses;
    vector<pcidevice_t*> devices;
    bool legacy = false;

    static void *get_addr(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset)
    {
        for (const auto &entry : mcfg_entries)
        {
            if (entry.segment == seg)
            {
                if ((bus >= entry.startbus) && (bus <= entry.endbus))
                {
                    return reinterpret_cast<void*>((entry.baseaddr + (((bus - entry.startbus) << 20) | (dev << 15) | (func << 12))) + offset);
                }
            }
        }
        return nullptr;
    }

    uint32_t read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width)
    {
        assert(width == 1 || width == 2 || width == 4, "Invalid width!");
        if (legacy)
        {
            uint32_t address = (bus << 16) | (dev << 11) | (func << 8) | (offset & ~(3)) | 0x80000000;
            outl(0xCF8, address);
            if (width == 1) return inb(0xCFC + (offset & 3));
            else if (width == 2) return inw(0xCFC + (offset & 3));
            else if (width == 4) return inl(0xCFC + (offset & 3));
        }
        else
        {
            if (width == 1) return mmin<uint8_t>(get_addr(seg, bus, dev, func, offset));
            else if (width == 2) return mmin<uint16_t>(get_addr(seg, bus, dev, func, offset));
            else if (width == 4) return mmin<uint32_t>(get_addr(seg, bus, dev, func, offset));
        }
        return 0;
    }

    void write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width)
    {
        assert(width == 1 || width == 2 || width == 4, "Invalid width!");
        if (legacy)
        {
            uint32_t address = (bus << 16) | (dev << 11) | (func << 8) | (offset & ~(3)) | 0x80000000;
            outl(0xCF8, address);
            if (width == 1) outb(0xCFC + (offset & 3), value);
            else if (width == 2) outw(0xCFC + (offset & 3), value);
            else if (width == 4) outl(0xCFC + (offset & 3), value);
        }
        else
        {
            if (width == 1) mmout<uint8_t>(get_addr(seg, bus, dev, func, offset), value);
            else if (width == 2) mmout<uint16_t>(get_addr(seg, bus, dev, func, offset), value);
            else if (width == 4) mmout<uint32_t>(get_addr(seg, bus, dev, func, offset), value);
        }
    }

    void pcidevice_t::msi_set(uint8_t vector)
    {
        uint16_t flags = acpi::fadthdr->BootArchitectureFlags;
        if (this->msi == false || flags & (1 << 3)) return;

        uint16_t msg_ctrl = this->read<uint16_t>(this->msi_offset + 2);

        this->write<uint32_t>(this->msi_offset + 0x04, (0x0FEE << 20) | (smp_request.response->bsp_lapic_id << 12));
        this->write<uint32_t>(this->msi_offset + (((msg_ctrl << 7) & 1) == 1 ? 0x0C : 0x08), vector);
        this->write<uint16_t>(this->msi_offset + 2, (msg_ctrl | 1) & ~(0b111 << 4));
    }

    void pcidevice_t::command(uint16_t cmd, bool enable)
    {
        uint16_t command = this->read<uint16_t>(PCI_COMMAND);

        if (enable) command |= cmd;
        else command &= ~cmd;

        this->write<uint16_t>(PCI_COMMAND, command);
    }

    std::tuple<bartype, uint64_t, uint64_t> pcidevice_t::get_bar(size_t num)
    {
        bartype bar_type = PCI_BARTYPE_INVALID;
        if (num > 5) return std::make_tuple(bar_type, 0, 0);

        uint32_t offset = PCI_BAR0 + num * 4;
        uint32_t bar = this->read<uint32_t>(offset);

        uint64_t base = 0;
        uint64_t length = 0;
        if (bar & 0x01)
        {
            bar_type = PCI_BARTYPE_IO;
            base = (bar & 0xFFFFFFFC) & 0xFFFF;

            this->write<uint32_t>(offset, 0xFFFFFFFF);
            length = (~((this->read<uint32_t>(offset) & ~0x03)) + 0x01) & 0xFFFF;
            this->write<uint32_t>(offset, bar);
        }
        else
        {
            bar_type = PCI_BARTYPE_MMIO;

            switch ((bar >> 1) & 0x03)
            {
                case 0x00:
                    base = (bar & 0xFFFFFFF0);
                    break;
                case 0x02:
                    base = ((bar & 0xFFFFFFF0) | (static_cast<uint64_t>(this->read<uint32_t>(offset + 0x04)) << 32));
                    break;
                default:
                    log::warn("PCI: Unknown MMIO bar type 0x%X", (bar >> 1) & 0x03);
                    break;
            }

            this->write<uint32_t>(offset, 0xFFFFFFFF);
            length = ~((this->read<uint32_t>(offset) & ~(0x0F))) + 0x01;
            this->write<uint32_t>(offset, bar);

            if (base == 0 && length == 0) bar_type = PCI_BARTYPE_INVALID;
        }

        return std::make_tuple(bar_type, base, length);
    }


    size_t eval_aml_method(lai_nsnode_t *node, const char *name)
    {
        LAI_CLEANUP_STATE lai_state_t state;
        lai_init_state(&state);

        LAI_CLEANUP_VAR lai_variable_t var;
        auto handle = lai_resolve_path(node, name);
        if (handle == nullptr)
        {
            log::warn("PCI: Root bus doesn't have %s, assuming 0", name);
            return 0;
        }

        if (auto e = lai_eval(&var, handle, &state); e != LAI_ERROR_NONE)
        {
            log::warn("PCI: Couldn't evaluate Root Bus %s, assuming 0", name);
            return 0;
        }

        uint64_t ret = 0;
        if (auto e = lai_obj_get_integer(&var, &ret); e != LAI_ERROR_NONE)
        {
            log::warn("PCI: Root Bus %s evaluation didn't result in an integer, assuming 0", name);
            return 0;
        }

        return ret;
    }

    static void enumbus(uint16_t seg, uint8_t bus, pcidevice_t *parent);
    static void enumfunc(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, pcidevice_t *parent)
    {
        uint16_t vendorid = read(seg, bus, dev, func, PCI_VENDOR_ID, 2);
        uint16_t deviceid = read(seg, bus, dev, func, PCI_DEVICE_ID, 2);
        if (vendorid == 0xFFFF || deviceid == 0xFFFF) return;

        auto device = new pcidevice_t
        {
            .vendorid = vendorid,
            .deviceid = deviceid,
            .progif = 0,
            .subclass = 0,
            .Class = 0,
            .seg = seg,
            .bus = bus,
            .dev = dev,
            .func = func,
            .parent = parent,
        };

        device->progif = device->read<uint8_t>(PCI_PROG_IF);
        device->subclass = device->read<uint8_t>(PCI_SUBCLASS);
        device->Class = device->read<uint8_t>(PCI_CLASS);

        if (device->Class == 0x06 && device->subclass == 0x04)
        {
            uint8_t secondary_bus = device->read<uint8_t>(PCI_SECONDARY_BUS);
            enumbus(seg, secondary_bus, device);
            device->bridge = true;
        }

        uint16_t status = device->read<uint16_t>(PCI_STATUS);
        if (status & (1 << 4))
        {
            uint8_t nextptr = device->read<uint8_t>(PCI_CAPABPTR);
            while (nextptr)
            {
                uint8_t id = device->read<uint8_t>(nextptr);

                switch (id)
                {
                    case 0x05:
                        device->msi = true;
                        device->msi_offset = nextptr;
                        break;
                }

                nextptr = device->read<uint8_t>(nextptr + 1);
            }
        }

        log::info("PCI: Found device: %X:%X", vendorid, deviceid);
        devices.push_back(device);
    }

    static void enumdev(uint16_t seg, uint8_t bus, uint8_t dev, pcidevice_t *parent)
    {
        uint16_t vendorid = read(seg, bus, dev, 0, PCI_VENDOR_ID, 2);
        if (vendorid == 0xFFFF) return;

        uint8_t header_type = read(seg, bus, dev, 0, PCI_HEADER_TYPE, 1);
        if (header_type & (1 << 7))
        {
            for (uint8_t i = 0; i < 8; i++) enumfunc(seg, bus, dev, i, parent);
        }
        else enumfunc(seg, bus, dev, 0, parent);
    }

    static void enumbus(uint16_t seg, uint8_t bus, pcidevice_t *parent)
    {
        for (uint8_t i = 0; i < 32; i++) enumdev(seg, bus, i, parent);
    }

    static pcidevice_t *get_root_bus(uint16_t seg, uint8_t bus)
    {
        for (auto it : root_buses)
        {
            if (seg == it->seg && bus == it->bus) return it;
        }
        return nullptr;
    }

    static void find_node(pcidevice_t *dev, lai_state_t *state)
    {
        if (dev->node) return;
        if (dev->parent) find_node(dev->parent, state);

        lai_nsnode_t *bus = nullptr;
        auto root = get_root_bus(dev->seg, dev->bus);
        if (root == nullptr)
        {
            assert(dev->parent, "PCI Device not on root bus does not have parent node");
            bus = dev->parent->node;
        }
        else bus = root->node;

        if (bus) dev->node = lai_pci_find_device(bus, dev->dev, dev->func, state);
    }

    static void route_irqs()
    {
        LAI_CLEANUP_STATE lai_state_t state;
        lai_init_state(&state);

        for (auto dev : devices)
        {
            if (dev->bridge) find_node(dev, &state);

            if (dev->node)
            {
                if (!lai_obj_get_type(&dev->prt))
                {
                    lai_nsnode_t *prt_handle = lai_resolve_path(dev->node, "_PRT");
                    if (prt_handle)
                    {
                        if (lai_eval(&dev->prt, prt_handle, &state))
                        {
                            log::error("PCI: Failed to evaluate PRT for %d:%d:%d:%d", dev->seg, dev->bus, dev->dev, dev->func);
                        }
                    }
                }
            }
        }

        for (auto dev : devices)
        {
            uint8_t irq_pin = dev->read<uint8_t>(PCI_INTERRUPT_PIN);
            if (irq_pin == 0 || irq_pin > 4) continue;

            pcidevice_t *tmp = dev;
            lai_variable_t* prt = nullptr;

            while (true)
            {
                if (tmp->parent)
                {
                    if (!lai_obj_get_type(&tmp->parent->prt)) irq_pin = (((irq_pin - 1) + (tmp->dev % 4)) % 4) + 1;
                    else
                    {
                        prt = &tmp->parent->prt;
                        break;
                    }
                    tmp = tmp->parent;
                }
                else
                {
                    pcidevice_t *bus = get_root_bus(tmp->seg, tmp->bus);
                    assert(bus, "Couldn't find root node for device on it");
                    prt = &bus->prt;
                    break;
                }
            }

            assert(prt, "Failed to find PRT");

            lai_prt_iterator iter = LAI_PRT_ITERATOR_INITIALIZER(prt);
            lai_api_error_t err;

            bool found = false;

            while (!(err = lai_pci_parse_prt(&iter)))
            {
                if (iter.slot == tmp->dev && (iter.function == tmp->func || iter.function == -1) && iter.pin == (irq_pin - 1))
                {
                    dev->gsi = iter.gsi;
                    dev->has_irq = true;
                    found = true;
                    break;
                }
            }

            if (found == false) log::error("PCI: Routing failed for %d:%d:%d:%d", dev->seg, dev->bus, dev->dev, dev->func);
        }
    }

    void init()
    {
        log::info("Initialising PCI...");

        if (acpi::mcfghdr == nullptr)
        {
            log::warn("PCI: MCFG table not found!");
            legacy = true;
        }
        else if (acpi::mcfghdr->header.length < sizeof(acpi::MCFGHeader) + sizeof(acpi::MCFGEntry))
        {
            log::error("PCI: No entries found in MCFG table!");
            legacy = true;
        }

        if (legacy == false)
        {
            size_t entries = ((acpi::mcfghdr->header.length) - sizeof(acpi::MCFGHeader)) / sizeof(acpi::MCFGEntry);
            for (size_t i = 0; i < entries; i++)
            {
                acpi::MCFGEntry &entry = acpi::mcfghdr->entries[i];
                mcfg_entries.push_back(entry);
            }
        }

        LAI_CLEANUP_STATE lai_state_t state;
        lai_init_state(&state);

        lai_variable_t pci_pnp_id = { };
        lai_variable_t pcie_pnp_id = { };
        lai_eisaid(&pci_pnp_id, ACPI_PCI_ROOT_BUS_PNP_ID);
        lai_eisaid(&pcie_pnp_id, ACPI_PCIE_ROOT_BUS_PNP_ID);

        auto _SB_ = lai_resolve_path(nullptr, "_SB_");
        assert(_SB_ != nullptr, "Couldn't resolve path \"_SB_\"!");

        lai_ns_child_iterator it = LAI_NS_CHILD_ITERATOR_INITIALIZER(_SB_);
        lai_nsnode_t *node = nullptr;
        while ((node = lai_ns_child_iterate(&it)))
        {
            if (lai_check_device_pnp_id(node, &pci_pnp_id, &state) && lai_check_device_pnp_id(node, &pcie_pnp_id, &state)) continue;

            auto seg = eval_aml_method(node, "_SEG");
            auto bbn = eval_aml_method(node, "_BBN");

            auto bus = new pcidevice_t
            {
                .seg = static_cast<uint16_t>(seg),
                .bus = static_cast<uint8_t>(bbn),
                .dev = 0,
                .func = 0,
                .parent = nullptr,
                .node = node,
                .prt = { }
            };

            auto prt_handle = lai_resolve_path(node, "_PRT");
            if (prt_handle != nullptr)
            {
                auto e = lai_eval(&bus->prt, prt_handle, &state);
                if (e != LAI_ERROR_NONE) log::warn("PCI: Couldn't evaluate Root Bus _PRT, assuming none\n");
            }
            else log::warn("PCI: Root bus doesn't have _PRT, assuming none\n");

            root_buses.push_back(bus);
            enumbus(seg, bbn, nullptr);
        }

        route_irqs();
    }
} // namespace pci

void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint8_t val)
{
    pci::write(seg, bus, slot, fun, offset, val, 1);
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint16_t val)
{
    pci::write(seg, bus, slot, fun, offset, val, 2);
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint32_t val)
{
    pci::write(seg, bus, slot, fun, offset, val, 4);
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return pci::read(seg, bus, slot, fun, offset, 1);
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return pci::read(seg, bus, slot, fun, offset, 2);
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return pci::read(seg, bus, slot, fun, offset, 4);
}