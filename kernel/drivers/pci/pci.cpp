// Copyright (C) 2022  ilobilo

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

    template<>
    uint8_t read<uint8_t>(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset)
    {
        configio *io = configspaces.at((seg << 8) | bus);
        assert(io != nullptr);

        return io->read<uint8_t>(seg, bus, dev, func, offset);
    }

    template<>
    uint16_t read<uint16_t>(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset)
    {
        configio *io = configspaces.at((seg << 8) | bus);
        assert(io != nullptr);

        return io->read<uint16_t>(seg, bus, dev, func, offset);
    }

    template<>
    uint32_t read<uint32_t>(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset)
    {
        configio *io = configspaces.at((seg << 8) | bus);
        assert(io != nullptr);

        return io->read<uint32_t>(seg, bus, dev, func, offset);
    }

    template<>
    void write<uint8_t>(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint8_t value)
    {
        configio *io = configspaces.at((seg << 8) | bus);
        assert(io != nullptr);

        io->write<uint8_t>(seg, bus, dev, func, offset, value);
    }

    template<>
    void write<uint16_t>(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint16_t value)
    {
        configio *io = configspaces.at((seg << 8) | bus);
        assert(io != nullptr);

        io->write<uint16_t>(seg, bus, dev, func, offset, value);
    }

    template<>
    void write<uint32_t>(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value)
    {
        configio *io = configspaces.at((seg << 8) | bus);
        assert(io != nullptr);

        io->write<uint32_t>(seg, bus, dev, func, offset, value);
    }

    void addconfigio(uint32_t seg, uint32_t bus, configio *io)
    {
        configspaces[(seg << 8) | bus] = io;
    }

    void addrootbus(bus_t *bus)
    {
        log::info("PCI: Found root bus 0x%lX:0x%lX", bus->seg, bus->bus);
        root_buses.push_back(bus);
    }

    static void readbars(entity *device, size_t count)
    {
        assert(device != nullptr);
        assert(count <= 6);

        auto bars = device->getbars();

        for (size_t num = 0; num < count; num++)
        {
            bar_t ret = { 0, 0, PCI_BARTYPE_INVALID, false };
            bool skipnext = false;

            uint32_t offset = PCI_BAR0 + num * 4;
            uint32_t bar = device->read<uint32_t>(offset);

            auto barlen = [&device, bar](uint32_t offset, uint32_t bits)
            {
                device->write<uint32_t>(offset, 0xFFFFFFFF);
                size_t length = (~(device->read<uint32_t>(offset) & ~bits) + 0x01) & 0xFFFF;
                device->write<uint32_t>(offset, bar);

                return length;
            };

            if (bar & 0x01)
            {
                uintptr_t addr = bar & 0xFFFFFFFC;
                size_t length = barlen(offset, 0b11);

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
                        length = barlen(offset, 0b1111);
                        addr = bar & 0xFFFFFFF0;
                        break;
                    case 0x02:
                        if (num == count - 1)
                            continue;

                        length = barlen(offset, 0b1111) | (static_cast<uint64_t>(barlen(offset + 0x04, 0b1111)) << 32);
                        addr = (bar & 0xFFFFFFF0) | (static_cast<uint64_t>(device->read<uint32_t>(offset + 0x04)) << 32);

                        skipnext = true;
                        break;
                    default:
                        log::warn("PCI: Unknown MMIO bar type 0x%X", type);
                        break;
                }

                ret.base = addr;
                ret.len = length;
                ret.type = PCI_BARTYPE_MMIO;
                ret.prefetchable = bar & (1 << 3);

                if (ret.base == 0 && ret.len == 0)
                    ret.type = PCI_BARTYPE_INVALID;
            }

            bars[skipnext ? num++ : num] = ret;
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
        uint16_t flags = acpi::fadthdr->BootArchitectureFlags;
        if (this->msi == 0 || flags & (1 << 3)) return false;

        msi::control control { .raw = this->read<uint16_t>(this->msi + 0x02) };
        msi::address address { .raw = this->read<uint16_t>(this->msi + 0x04) };
        msi::data data { .raw = this->read<uint16_t>(this->msi + (control.c64 ? 0xC0 : 0x08)) };

        data.vector = vector;
        data.delivery_mode = 0;

        address.base_address = 0xFEE;
        address.destination_id = cpuid;

        this->write<uint16_t>(this->msi + 0x04, address.raw);
        this->write<uint16_t>(this->msi + (control.c64 ? 0xC0 : 0x08), data.raw);

        control.msie = 1;
        control.mme = 0b000;

        this->write<uint16_t>(this->msi + 0x02, control.raw);

        return true;
    }

    bool msix_set(uint64_t cpuid, uint16_t vector, uint16_t index)
    {
        log::error("PCI: TODO: MSI-X is not supported!");
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
            if (offset != 0)
                log::info(" Capabilities:");

            while (offset)
            {
                uint16_t entry = device->read<uint16_t>(offset);
                uint8_t id = entry & 0xFF;

                switch (id)
                {
                    case 0x10:
                    {
                        log::info("  - PCIe");
                        auto tp = (device->read<uint16_t>(offset + 2) >> 4) & 0x0F;
                        device->is_pcie = true;
                        device->is_secondary = (tp == 4 || tp == 6 || tp == 8);
                        break;
                    }
                    default:
                        if (func(id, offset) != true)
                            log::info("  - Unknown: 0x%X", id);
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
        if (vendorid == 0xFFFF || deviceid == 0xFFFF) return;

        uint8_t header_type = bus->read<uint8_t>(dev, func, PCI_HEADER_TYPE) & 0x7F;
        if (header_type == 0x00)
        {
            log::info("PCI: Found device: %.4X:%.4X", vendorid, deviceid);

            uint8_t progif = bus->read<uint8_t>(dev, func, PCI_PROG_IF);
            uint8_t subclass = bus->read<uint8_t>(dev, func, PCI_SUBCLASS);
            uint8_t Class = bus->read<uint8_t>(dev, func, PCI_CLASS);

            auto device = new device_t(vendorid, deviceid, progif, subclass, Class, bus->seg, bus->bus, dev, func, bus);

            capabilities(device, [&](uint8_t id, uint8_t offset) -> bool
            {
                switch (id)
                {
                    case 0x5:
                        log::info("  - MSI");
                        device->msi = offset;
                        break;
                    case 0x11:
                        log::info("  - MSI-X");
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
            log::info("PCI: Found PCI-to-PCI bridge: %.4X:%.4X", vendorid, deviceid);
            auto bridge = new bridge_t(bus->seg, bus->bus, dev, func, bus);

            capabilities(bridge, [&](uint8_t id, uint8_t offset) -> bool { return true; });

            uint8_t secondary_id = bridge->read<uint8_t>(PCI_SECONDARY_BUS);
            if (secondary_id != 0)
            {
                bridge->secondaryid = secondary_id;
                bridge->subordinateid = bridge->read<uint8_t>(PCI_SUBORDINATE_BUS);

                auto secondary_bus = new bus_t(bridge, bus->io, bus->seg, secondary_id);
                bridge->bus = secondary_bus;
                enumbus(secondary_bus);
            }

            bus->child_bridges.push_back(bridge);
        }
    }

    static void enumdev(bus_t *bus, uint8_t dev)
    {
        uint16_t vendorid = bus->read<uint16_t>(dev, 0, PCI_VENDOR_ID);
        if (vendorid == 0xFFFF) return;

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
        for (uint8_t i = 0; i < ((bus->bridge && bus->bridge->is_pcie && bus->bridge->is_secondary) ? 1 : 32); i++)
            enumdev(bus, i);
    }

    void init()
    {
        log::info("Initialising PCI...");

        if (arch::pci_init)
            arch::pci_init();

        assert(configspaces.size() != 0, "PCI: No config spaces found!");
        assert(root_buses.size() != 0, "PCI: No root buses found!");

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