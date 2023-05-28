// Copyright (C) 2022-2023  ilobilo

#include <drivers/pci/pci.hpp>
#include <drivers/acpi.hpp>
#include <arch/arch.hpp>
#include <lib/panic.hpp>
#include <lib/log.hpp>

namespace pci
{
    std::unordered_map<uint32_t, configio*> configspaces;
    std::vector<device_t*> devices;
    std::vector<bus_t*> root_buses;

    void addconfigio(uint32_t seg, uint32_t bus, configio *io)
    {
        configspaces[(seg << 8) | bus] = io;
    }

    void addrootbus(bus_t *bus)
    {
        log::infoln("PCI: Found root bus 0x{:X}:0x{:X}", bus->seg, bus->bus);
        root_buses.push_back(bus);
    }

    static void readbars(entity *device, size_t count)
    {
        assert(device != nullptr);
        assert(count <= 6);

        auto bars = device->getbars();

        for (size_t num = 0; num < count; num++)
        {
            bar_t ret = { 0, 0, PCI_BARTYPE_INVALID, false, false };
            bool bit64 = false;

            auto offset = PCI_BAR0 + num * sizeof(uint32_t);
            auto bar = device->read<uint32_t>(offset);

            device->write<uint32_t>(offset, 0xFFFFFFFF);
            auto lenlow = device->read<uint32_t>(offset);
            device->write<uint32_t>(offset, bar);

            if (bar & 0x01)
            {
                uintptr_t addr = bar & ~0b11;
                size_t length = (~(lenlow & ~0b11) + 1) & 0xFFFF;

                ret.base = addr;
                ret.len = length;
                ret.type = PCI_BARTYPE_IO;
                ret.prefetchable = false;
            }
            else
            {
                size_t length = 0;
                uintptr_t addr = 0;

                switch (auto type = (bar >> 1) & 0x03)
                {
                    case 0x00:
                        length = ~(lenlow & ~0b1111) + 1;
                        addr = bar & ~0b1111;
                        break;
                    case 0x02:
                    {
                        if (num == count - 1)
                            continue;

                        auto offseth = offset + sizeof(uint32_t);
                        auto barh = device->read<uint32_t>(offseth);

                        device->write<uint32_t>(offseth, 0xFFFFFFFF);
                        auto lenhigh = device->read<uint32_t>(offseth);
                        device->write<uint32_t>(offseth, barh);

                        length = ~((uint64_t(lenhigh) << 32) | (lenlow & ~0b1111)) + 1;
                        addr = (uint64_t(barh) << 32) | (bar & ~0b1111);

                        bit64 = true;
                        break;
                    }
                    default:
                        log::warnln("PCI: Unknown MMIO bar type 0x{:X}", type);
                        break;
                }

                ret.base = addr;
                ret.len = length;
                ret.type = PCI_BARTYPE_MMIO;
                ret.prefetchable = bar & (1 << 3);
                ret.bit64 = bit64;

                if (ret.base == 0 && ret.len == 0)
                    ret.type = PCI_BARTYPE_INVALID;
            }
            bars[num] = ret;
            if (bit64 == true)
                bars[++num] = { 0, 0, PCI_BARTYPE_INVALID, false, false };
        }
    }

    bridge_t::bridge_t(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, bus_t *parent) : entity(seg, bus, dev, func, parent)
    {
        readbars(this, 2);
    }

    device_t::device_t(uint16_t vendorid, uint16_t deviceid, uint8_t progif, uint8_t subclass, uint8_t Class, uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, bus_t *parent)
    : entity(seg, bus, dev, func, parent), vendorid(vendorid), deviceid(deviceid), progif(progif), subclass(subclass), Class(Class)
    {
        readbars(this, 6);
    }

    bool device_t::msi_set(uint64_t cpuid, uint16_t vector, uint16_t index)
    {
        uint16_t flags = ::acpi::fadthdr->BootArchitectureFlags;
        if (this->msi == 0 || flags & (1 << 3))
            return false;

        msi::control control { .raw = this->read<uint16_t>(this->msi + 0x02) };
        msi::address address { .raw = this->read<uint16_t>(this->msi + 0x04) };
        msi::data data { .raw = this->read<uint16_t>(this->msi + (control.c64 ? 0xC0 : 0x08)) };

        data.vector = vector;
        data.delivery_mode = 0;

        address.base_address = 0xFEE;
        address.destination_id = cpuid;

        this->write<uint16_t>(this->msi + 0x04, address.raw);
        this->write<uint16_t>(this->msi + (control.c64 ? 0x0C : 0x08), data.raw);

        control.msie = 1;
        control.mme = 0b000;

        this->write<uint16_t>(this->msi + 0x02, control.raw);

        return true;
    }

    bool msix_set(uint64_t cpuid, uint16_t vector, uint16_t index)
    {
        log::errorln("PCI: TODO: MSI-X is not supported!");
        return false;
    }

    void entity::command(uint16_t cmd, bool enable)
    {
        uint16_t command = this->read<uint16_t>(PCI_COMMAND);

        if (enable) command |= cmd;
        else command &= ~cmd;

        this->write<uint16_t>(PCI_COMMAND, command);
    }

    static void capabilities(entity *device, auto func)
    {
        uint16_t status = device->read<uint16_t>(PCI_STATUS);
        if (status & (1 << 4))
        {
            uint8_t offset = device->read<uint16_t>(PCI_CAPABPTR) & 0xFC;
            // if (offset != 0)
            //     log::infoln(" Capabilities:");

            while (offset)
            {
                uint16_t entry = device->read<uint16_t>(offset);
                uint8_t id = entry & 0xFF;

                switch (id)
                {
                    case 0x10:
                    {
                        // log::infoln("  - PCIe");
                        auto tp = (device->read<uint16_t>(offset + 2) >> 4) & 0x0F;
                        device->is_pcie = true;
                        device->is_secondary = (tp == 4 || tp == 6 || tp == 8);
                        break;
                    }
                    default:
                        // if (func(id, offset) != true)
                        //     log::infoln("  - Unknown: 0x{:X}", id);
                        break;
                }

                offset = (entry >> 8) & 0xFC;
            }
        }
    };

    static void enumbus(bus_t *bus);
    static void enumfunc(bus_t *bus, uint8_t dev, uint8_t func)
    {
        uint16_t vendorid = bus->read<uint16_t>(dev, func, PCI_VENDOR_ID);
        uint16_t deviceid = bus->read<uint16_t>(dev, func, PCI_DEVICE_ID);
        if (vendorid == 0xFFFF || deviceid == 0xFFFF)
            return;

        uint8_t header_type = bus->read<uint8_t>(dev, func, PCI_HEADER_TYPE) & 0x7F;
        if (header_type == 0x00)
        {
            log::infoln("PCI: Found device: {:04X}:{:04X}", vendorid, deviceid);

            uint8_t progif = bus->read<uint8_t>(dev, func, PCI_PROG_IF);
            uint8_t subclass = bus->read<uint8_t>(dev, func, PCI_SUBCLASS);
            uint8_t Class = bus->read<uint8_t>(dev, func, PCI_CLASS);

            auto device = new device_t(vendorid, deviceid, progif, subclass, Class, bus->seg, bus->bus, dev, func, bus);

            capabilities(device, [&](uint8_t id, uint8_t offset) -> bool
            {
                switch (id)
                {
                    case 0x5:
                        // log::infoln("  - MSI");
                        device->msi = offset;
                        break;
                    case 0x11:
                        // log::infoln("  - MSI-X");
                        device->msix = offset;
                        break;
                    default:
                        return false;
                }
                return true;
            });

            devices.push_back(device);
            bus->child_devices.push_back(device);
        }
        else if (header_type == 0x01)
        {
            log::infoln("PCI: Found PCI-to-PCI bridge: {:04X}:{:04X}", vendorid, deviceid);
            auto bridge = new bridge_t(bus->seg, bus->bus, dev, func, bus);

            capabilities(bridge, [&](uint8_t id, uint8_t offset) -> bool { return true; });

            uint8_t secondary_id = bridge->read<uint8_t>(PCI_SECONDARY_BUS);
            if (secondary_id != 0)
            {
                bridge->secondaryid = secondary_id;
                bridge->subordinateid = bridge->read<uint8_t>(PCI_SUBORDINATE_BUS);

                auto secondary_bus = new bus_t(bridge, bus->io, bus->seg, secondary_id);
                bridge->parent = secondary_bus;
                enumbus(secondary_bus);
            }

            bus->child_bridges.push_back(bridge);
        }
    }

    static void enumdev(bus_t *bus, uint8_t dev)
    {
        uint16_t vendorid = bus->read<uint16_t>(dev, 0, PCI_VENDOR_ID);
        if (vendorid == 0xFFFF)
            return;

        uint8_t header_type = bus->read<uint8_t>(dev, 0, PCI_HEADER_TYPE);
        if (header_type & (1 << 7))
        {
            for (uint8_t i = 0; i < 8; i++)
                enumfunc(bus, dev, i);
        }
        else enumfunc(bus, dev, 0);
    }

    static void enumbus(bus_t *bus)
    {
        auto devs = ((bus->bridge && bus->bridge->is_pcie && bus->bridge->is_secondary) ? 1 : 32);
        for (uint8_t i = 0; i < devs; i++)
            enumdev(bus, i);
    }

    void init()
    {
        log::infoln("PCI: Initialising...");

        if (pci::arch_init)
            pci::arch_init();

        if (configspaces.size() == 0)
        {
            log::errorln("PCI: No config spaces found!");
            return;
        }

        if (root_buses.size() == 0)
        {
            log::errorln("PCI: No root buses found!");
            return;
        }

        for (const auto bus : root_buses)
            enumbus(bus);
    }
} // namespace pci

void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint8_t val)
{
    pci::write<uint8_t>(seg, bus, slot, fun, offset, val);
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint16_t val)
{
    pci::write<uint16_t>(seg, bus, slot, fun, offset, val);
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint32_t val)
{
    pci::write<uint32_t>(seg, bus, slot, fun, offset, val);
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return pci::read<uint8_t>(seg, bus, slot, fun, offset);
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return pci::read<uint16_t>(seg, bus, slot, fun, offset);
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return pci::read<uint32_t>(seg, bus, slot, fun, offset);
}