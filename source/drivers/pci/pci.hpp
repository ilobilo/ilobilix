// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/tuple.hpp>
#include <type_traits>
#include <lai/core.h>
#include <cstdint>

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
        PCI_CAPABPTR = 0x34,
        PCI_INTERRUPT_LINE = 0x3C,
        PCI_INTERRUPT_PIN = 0x3D
    };

    uint32_t read(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, size_t width = 8);
    void write(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func, size_t offset, uint32_t value, size_t width = 8);

    template<typename type>
    concept valuetype = (std::is_same<type, uint8_t>() || std::is_same<type, uint16_t>() || std::is_same<type, uint32_t>());

    struct pcidevice_t
    {
        uint16_t vendorid;
        uint16_t deviceid;

        uint8_t progif;
        uint8_t subclass;
        uint8_t Class;

        uint16_t seg;
        uint8_t bus;
        uint8_t dev;
        uint8_t func;

        pcidevice_t *parent;
        lai_nsnode_t *node;
        lai_variable_t prt;

        bool msi = false;
        uint16_t msi_offset;

        bool has_irq = false;
        uint32_t gsi;

        bool bridge = false;

        template<valuetype type>
        type read(size_t offset)
        {
            return static_cast<type>(pci::read(this->seg, this->bus, this->dev, this->func, offset, sizeof(type)));
        }

        template<valuetype type>
        void write(size_t offset, type value)
        {
            pci::write(this->seg, this->bus, this->dev, this->func, offset, value, sizeof(type));
        }

        void msi_set(uint8_t vector);

        void command(uint16_t cmd, bool enable = true);
        std::tuple<uint64_t, bool> get_bar(size_t bar);
    };

    void init();
} // namespace pci