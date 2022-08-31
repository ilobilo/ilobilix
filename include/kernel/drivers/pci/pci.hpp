// Copyright (C) 2022  ilobilo

#pragma once

#include <unordered_map>
#include <type_traits>
#include <lai/core.h>
#include <cstdint>
#include <vector>

namespace pci
{
    enum cmds
    {
        CMD_IO_SPACE = (1 << 0),
        CMD_MEM_SPACE = (1 << 1),
        CMD_BUS_MAST = (1 << 2),
        CMD_SPEC_CYC = (1 << 3),
        CMD_MEM_WRITE = (1 << 4),
        CMD_VGA_PS = (1 << 5),
        CMD_PAR_ERR = (1 << 6),
        CMD_SERR = (1 << 8),
        CMD_FAST_B2B = (1 << 9),
        CMD_INT_DIS = (1 << 10),
    };

    enum offsets
    {
        PCI_VENDOR_ID = 0x00,
        PCI_DEVICE_ID = 0x02,
        PCI_COMMAND = 0x04,
        PCI_STATUS = 0x06,
        PCI_REVISION_ID = 0x08,
        PCI_PROG_IF = 0x09,
        PCI_SUBCLASS = 0x0A,
        PCI_CLASS = 0x0B,
        PCI_CACHE_LINE_SIZE = 0x0C,
        PCI_LATENCY_TIMER = 0x0D,
        PCI_HEADER_TYPE = 0x0E,
        PCI_BIST = 0x0F,
        PCI_BAR0 = 0x10,
        PCI_BAR1 = 0x14,
        PCI_BAR2 = 0x18,
        PCI_BAR3 = 0x1C,
        PCI_BAR4 = 0x20,
        PCI_BAR5 = 0x24,
        PCI_SECONDARY_BUS = 0x19,
        PCI_SUBORDINATE_BUS = 0x1A,
        PCI_CAPABPTR = 0x34,
        PCI_INTERRUPT_LINE = 0x3C,
        PCI_INTERRUPT_PIN = 0x3D
    };

    enum bartype
    {
        PCI_BARTYPE_MMIO = 0,
        PCI_BARTYPE_IO = 1,
        PCI_BARTYPE_INVALID = 2
    };

    template<typename Type>
    concept valuetype = (std::is_same_v<Type, uint8_t> || std::is_same_v<Type, uint16_t> || std::is_same_v<Type, uint32_t>);

    struct [[gnu::aligned]] configio
    {
        template<valuetype Type>
        Type read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset)
        {
            return this->read(seg, bus, dev, func, offset, sizeof(Type));
        }

        template<valuetype Type>
        void write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, Type value)
        {
            this->write(seg, bus, dev, func, offset, value, sizeof(Type));
        }

        protected:
        virtual uint32_t read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width) = 0;
        virtual void write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width) = 0;
    };

    template<valuetype Type>
    Type read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset);

    template<valuetype Type>
    void write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, Type value);

    struct bar_t
    {
        uintptr_t base;
        size_t len;
        bartype type;
        bool prefetchable;
    };

    namespace msi
    {
        union [[gnu::packed]] control
        {
            struct
            {
                uint16_t msie : 1;
                uint16_t mmc : 3;
                uint16_t mme : 3;
                uint16_t c64 : 1;
                uint16_t pvm : 1;
                uint16_t reserved : 6;
            };
            uint16_t raw;
        };

        union [[gnu::packed]] address
        {
            struct
            {
                uint32_t reserved : 2;
                uint32_t destination_mode : 1;
                uint32_t redirection_hint : 1;
                uint32_t reserved_0 : 8;
                uint32_t destination_id : 8;
                uint32_t base_address : 12;
            };
            uint32_t raw;
        };

        union [[gnu::packed]] data
        {
            struct
            {
                uint32_t vector : 8;
                uint32_t delivery_mode : 3;
                uint32_t reserved : 3;
                uint32_t level : 1;
                uint32_t trigger_mode : 1;
                uint32_t reserved_0 : 16;
            };
            uint32_t raw;
        };
    } // namespace msi

    struct bridge_t;
    struct device_t;

    struct bus_t
    {
        std::vector<device_t*> child_devices;
        std::vector<bridge_t*> child_bridges;

        bridge_t *bridge;
        configio *io;
        uint16_t seg;
        uint8_t bus;

        bus_t(bridge_t *bridge, configio *io, uint16_t seg, uint8_t bus) : bridge(bridge), io(io), seg(seg), bus(bus) { }

        template<valuetype Type>
        Type read(uint8_t dev, uint8_t func, size_t offset)
        {
            return this->io->read<Type>(this->seg, this->bus, dev, func, offset);
        }

        template<valuetype Type>
        void write(uint8_t dev, uint8_t func, size_t offset, Type value)
        {
            this->io->write<Type>(this->seg, this->bus, dev, func, offset, value);
        }
    };

    struct entity
    {
        uint16_t seg;
        uint8_t bus;
        uint8_t dev;
        uint8_t func;

        bus_t *parent;

        bool is_pcie;
        bool is_secondary;

        entity(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, bus_t *parent) : seg(seg), bus(bus), dev(dev), func(func), parent(parent) { }

        template<valuetype Type>
        Type read(size_t offset)
        {
            return this->parent->io->read<Type>(this->seg, this->bus, this->dev, this->func, offset);
        }

        template<valuetype Type>
        void write(size_t offset, Type value)
        {
            this->parent->io->write<Type>(this->seg, this->bus, this->dev, this->func, offset, value);
        }

        void command(uint16_t cmd, bool enable = true);
        virtual bar_t *getbars() = 0;
    };

    struct bridge_t : entity
    {
        bus_t *bus;
        uint8_t secondaryid;
        uint8_t subordinateid;

        bar_t bars[2];

        bridge_t(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, bus_t *parent);

        bar_t *getbars()
        {
            return this->bars;
        }
    };

    struct device_t : entity
    {
        uint16_t vendorid;
        uint16_t deviceid;

        uint8_t progif;
        uint8_t subclass;
        uint8_t Class;

        bar_t bars[6];

        uint8_t msi = 0;
        uint8_t msix = 0;

        uint32_t gsi = 0;

        bool msi_set(uint64_t cpuid, uint16_t vector, uint16_t index);
        bool msix_set(uint64_t cpuid, uint16_t vector, uint16_t index);

        device_t(uint16_t vendorid, uint16_t deviceid, uint8_t progif, uint8_t subclass, uint8_t Class, uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, bus_t *parent);

        bar_t *getbars()
        {
            return this->bars;
        }
    };

    extern std::unordered_map<uint32_t, configio*> configspaces;
    extern std::vector<device_t*> devices;
    extern std::vector<bus_t*> root_buses;

    inline configio *getconfigio(uint32_t seg, uint32_t bus)
    {
        return configspaces.at((seg << 8) | bus);
    }

    void addconfigio(uint32_t seg, uint32_t bus, configio *io);
    void addrootbus(bus_t *bus);

    void init();
    [[gnu::weak]] void arch_init();
} // namespace pci