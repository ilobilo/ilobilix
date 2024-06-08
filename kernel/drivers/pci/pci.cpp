// Copyright (C) 2022-2024  ilobilo

#include <drivers/pci/pci.hpp>
#include <drivers/acpi.hpp>
#include <arch/arch.hpp>

#include <lib/interrupts.hpp>
#include <lib/panic.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>

#include <mm/vmm.hpp>

#if defined(__x86_64__)
#  include <arch/x86_64/cpu/ioapic.hpp>
#  include <arch/x86_64/cpu/idt.hpp>
#  include <drivers/smp.hpp>
#endif

namespace pci
{
    std::unordered_map<uint32_t, configio*> configspaces;
    static std::vector<device_t*> devices;
    static std::vector<bus_t*> root_buses;

    std::vector<device_t*> &get_devices()
    {
        return devices;
    }

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
            bar_t ret = { vmm::invalid_addr, vmm::invalid_addr, 0, PCI_BARTYPE_INVALID, false, false };
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
                ret.pbase = addr;
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

                ret.base = vmm::invalid_addr;
                ret.pbase = addr;
                ret.len = length;
                ret.type = PCI_BARTYPE_MMIO;
                ret.prefetchable = bar & (1 << 3);
                ret.bit64 = bit64;

                if (ret.pbase == 0 && ret.len == 0)
                {
                    ret.pbase = vmm::invalid_addr;
                    ret.type = PCI_BARTYPE_INVALID;
                }
            }

            bars[num] = ret;
            if (bit64 == true)
                bars[++num] = { vmm::invalid_addr, vmm::invalid_addr, 0, PCI_BARTYPE_INVALID, false, true };
        }
    }

    uintptr_t bar_t::map(size_t alignment)
    {
        if (this->type != PCI_BARTYPE_MMIO)
            return vmm::invalid_addr;

        if (this->base != vmm::invalid_addr)
            return this->base;

        // if (this->bit64 == true)
        {
            if (vmm::kernel_pagemap->virt2phys(tohh(this->pbase)) == vmm::invalid_addr)
            {
                auto psize = vmm::kernel_pagemap->get_psize();

                auto albase = align_down(this->pbase, psize);
                auto len = align_up(this->pbase + this->len, psize) - albase;

                auto vaddr = vmm::alloc_vspace(vmm::vsptypes::bars, len, alignment ?: psize, !this->bit64);

                vmm::kernel_pagemap->map_range(vaddr, albase, len, vmm::rw, vmm::mmio);
                this->base = vaddr + (this->pbase - albase);
            }
            else this->base = tohh(this->pbase);
        }
        // TODO: map 32 bit bars too
        // else this->base = tohh(this->pbase);

        return this->base;
    }

    bridge_t::bridge_t(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, bus_t *parent) : entity(seg, bus, dev, func, parent)
    {
        readbars(this, 2);
    }

    device_t::device_t(uint16_t vendorid, uint16_t deviceid, uint8_t progif, uint8_t subclass, uint8_t Class, uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, bus_t *parent)
    : entity(seg, bus, dev, func, parent), vendorid(vendorid), deviceid(deviceid), progif(progif), subclass(subclass), Class(Class), route(nullptr), irq_registered(false), irq_index(0)
    {
        readbars(this, 6);
    }

    bool device_t::msi_set(uint64_t cpuid, uint16_t vector, uint16_t index)
    {
        if (this->msi.supported == false)
            return false;

        if (index == uint16_t(-1))
            index = 0; // TODO: Multiple

        msi::control control { .raw = this->read<uint16_t>(this->msi.offset + 0x02) };
        assert((1 << control.mmc) < 32);

        msi::data data { };
        msi::address address { };

        data.vector = vector;
        data.delivery_mode = 0;

        address.base_address = 0xFEE;
        address.destination_id = cpuid;

        this->write<uint16_t>(this->msi.offset + 0x04, address.raw);
        this->write<uint16_t>(this->msi.offset + (control.c64 ? 0x0C : 0x08), data.raw);

        control.msie = 1;
        control.mme = 0b000;

        this->write<uint16_t>(this->msi.offset + 0x02, control.raw);

        return true;
    }

    bool device_t::msix_set(uint64_t cpuid, uint16_t vector, uint16_t index)
    {
        if (this->msix.supported == false)
            return false;

        if (index == uint16_t(-1))
            index = this->msix.allocate_index();
        if (index == uint16_t(-1))
            return false;

        this->msix.irqs[index] = true;

        msix::control control { .raw = this->read<uint16_t>(this->msix.offset + 0x02) };
        control.mask = 1;
        control.enable = 1;
        this->write<uint16_t>(this->msix.offset + 0x02, control.raw);

        auto table_bar = this->bars[this->msix.table.bar];
        assert(table_bar.type == PCI_BARTYPE_MMIO);

        auto base = table_bar.map() + this->msix.table.offset;

        volatile auto *table = reinterpret_cast<volatile msix::entry*>(tohh(base));

        msi::data data { };
        msi::address address { };

        data.vector = vector;
        data.delivery_mode = 0;

        address.base_address = 0xFEE;
        address.destination_id = cpuid;

        msix::vectorctrl vectorctrl { };
        vectorctrl.mask = 0;

        table[index].addrlow = address.raw;
        table[index].addrhigh = 0; // TODO
        table[index].data = data.raw;
        table[index].control = vectorctrl.raw;

        control.raw = this->read<uint16_t>(this->msix.offset + 0x02);
        control.mask = 0;
        this->write<uint16_t>(this->msix.offset + 0x02, control.raw);

        return true;
    }

//     bool device_t::enable_irqs(uint64_t cpuid, size_t vector)
//     {
//         if (this->msix_set(cpuid, vector, -1))
//             return true;

//         if (this->msi_set(cpuid, vector, -1))
//             return true;

// #if defined(__x86_64__)
//         if (ioapic::initialised == false)
//         {
//             // this->write<uint8_t>(PCI_INTERRUPT_LINE, vector);
//             // idt::unmask(vector - 0x20);
//             // return true;
//             return false;
//         }
//         if (this->route == nullptr)
//             return false;

//         auto ioapic_flags = 0;
//         if (!(this->route->flags & ACPI_SMALL_IRQ_EDGE_TRIGGERED))
//             ioapic_flags |= ioapic::flags::level_sensative;
//         if (this->route->flags & ACPI_SMALL_IRQ_ACTIVE_LOW)
//             ioapic_flags |= ioapic::flags::active_low;

//         log::errorln("{}", this->route->gsi);
//         ioapic::set(this->route->gsi, vector, ioapic::delivery::fixed, ioapic::destmode::physical, ioapic_flags, smp::bsp_id);
//         return true;
// #endif
//         return false;
//     }

#if defined(__x86_64__)
    struct entry
    {
        std::vector<std::pair<device_t *, std::function<void ()>>> functions;
        size_t vector;

        void handler()
        {
            if (this->functions.size() == 1)
            {
                this->functions[0].second();
                return;
            }
            else if (this->functions.size() > 1)
            {
                for (auto &[dev, func] : this->functions)
                {
                    auto status = dev->read<uint16_t>(PCI_STATUS);
                    auto cmd = dev->read<uint16_t>(PCI_COMMAND);
                    if ((status & (1 << 3)) && !(cmd & CMD_INT_DIS))
                        func();
                }
                return;
            }
        }
    };
    static std::unordered_map<uint32_t, entry> gsis;
#endif

    bool device_t::register_irq(uint64_t cpuid, std::function<void ()> function)
    {
        auto [handler, vector] = interrupts::allocate_handler();

        if (this->msix_set(cpuid, vector, -1))
        {
            handler.set([&function] (auto) { function(); });
            return this->irq_registered = true;
        }

        if (this->msi_set(cpuid, vector, -1))
        {
            handler.set([&function] (auto) { function(); });
            return this->irq_registered = true;
        }

#if defined(__x86_64__)
        if (ioapic::initialised == false)
            goto exit;

        if (this->route == nullptr)
            goto exit;

        if (gsis.contains(this->route->gsi) == false)
        {
            auto &ref = gsis[this->route->gsi];
            ref.vector = vector;
            handler.set([&] (auto) { ref.handler(); });

            uint16_t ioapic_flags = 0;
            if (this->route->flags & pci::irq_router::flags::level)
                ioapic_flags |= ioapic::flags::level_sensative;
            if (this->route->flags & pci::irq_router::flags::low)
                ioapic_flags |= ioapic::flags::active_low;

            ioapic::set(this->route->gsi, vector, ioapic::delivery::fixed, ioapic::destmode::physical, ioapic_flags, smp::bsp_id);
        }
        else handler.reset();

        this->irq_index = gsis[this->route->gsi].functions.size();
        gsis[this->route->gsi].functions.emplace_back(this, function);
        return this->irq_registered = true;

        exit:
#endif
        handler.reset();
        return this->irq_registered = false;
    }

    bool device_t::unregister_irq()
    {
        if (this->irq_registered == false)
            return false;

        // TODO: MSI/-X

#if defined(__x86_64__)
        auto &ref = gsis[this->route->gsi];
        if (ref.functions.size() == 1)
        {
            interrupts::get_handler(ref.vector).reset();
            ref.functions.clear();
            gsis.erase(this->route->gsi);
        }
        else ref.functions.erase(std::next(ref.functions.begin(), this->irq_index));
#endif

        return true;
    }

    void entity::command(uint16_t cmd, bool enable)
    {
        uint16_t command = this->read<uint16_t>(PCI_COMMAND);

        if (enable == true)
            command |= cmd;
        else
            command &= ~cmd;

        this->write<uint16_t>(PCI_COMMAND, command);
    }

    static void capabilities(entity *device, auto func)
    {
        uint16_t status = device->read<uint16_t>(PCI_STATUS);
        if (status & (1 << 4))
        {
            uint8_t offset = device->read<uint16_t>(PCI_CAPABPTR) & 0xFC;
            if (offset != 0)
                log::infoln(" Capabilities:");

            while (offset)
            {
                uint16_t entry = device->read<uint16_t>(offset);
                uint8_t id = entry & 0xFF;

                switch (id)
                {
                    case 0x10:
                    {
                        log::infoln("  - PCIe");
                        auto tp = (device->read<uint16_t>(offset + 2) >> 4) & 0x0F;
                        device->is_pcie = true;
                        device->is_secondary = (tp == 4 || tp == 6 || tp == 8);
                        break;
                    }
                    default:
                        if (func(id, offset) != true)
                            log::infoln("  - Unknown: 0x{:X}", id);
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
            auto fadt = ::acpi::get_fadt();

            capabilities(device, [&](uint8_t id, uint8_t offset) -> bool
            {
                uint16_t flags = 0;
                if (fadt != nullptr)
                {
                    flags = fadt->iapc_boot_arch;
                    if (flags & (1 << 3))
                        return false;
                }

                switch (id)
                {
                    case 0x5:
                        if (flags & (1 << 3))
                        {
                            log::warnln("  - MSI (Supported but not available)");
                            device->msi.supported = false;
                            break;
                        }
                        log::infoln("  - MSI");
                        device->msi.supported = true;
                        device->msi.offset = offset;
                        break;
                    case 0x11:
                    {
                        if (flags & (1 << 3))
                        {
                            log::warnln("  - MSI-X (Supported but not available)");
                            device->msix.supported = false;
                            break;
                        }
                        log::infoln("  - MSI-X");
                        device->msix.supported = true;
                        device->msix.offset = offset;

                        msix::control control { .raw = device->read<uint16_t>(offset + 0x02) };
                        msix::address table { .raw = device->read<uint32_t>(offset + 0x04) };
                        msix::address pending { .raw = device->read<uint32_t>(offset + 0x08) };

                        size_t count = control.irqs;
                        device->msix.messages = count;
                        device->msix.irqs.allocate(count);

                        device->msix.table.bar = table.bir;
                        device->msix.table.offset = table.offset << 3;

                        device->msix.pending.bar = pending.bir;
                        device->msix.pending.offset = pending.offset << 3;
                        break;
                    }
                    default:
                        return false;
                }
                return true;
            });

            auto pin = device->read<uint8_t>(PCI_INTERRUPT_PIN);
            if (pin != 0 && bus->router != nullptr)
                device->route = bus->router->resolve(dev, pin);

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

                auto secondary_bus = new bus_t(bridge, nullptr, bus->io, bus->seg, secondary_id);
                auto router = bus->router->downstream(secondary_bus);
                secondary_bus->router = router;

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

        if (configspaces.empty())
        {
            log::errorln("PCI: No config spaces found");
            return;
        }

        if (root_buses.empty())
        {
            log::errorln("PCI: No root buses found");
            return;
        }

        for (const auto bus : root_buses)
            enumbus(bus);

        // TODO: Configure bridges from managarm
    }
} // namespace pci