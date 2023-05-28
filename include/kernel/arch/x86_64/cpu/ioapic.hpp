// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>

namespace ioapic
{
    enum class deliveryMode : uint8_t
    {
        FIXED = 0b000,
        LOW_PRIORITY = 0b001,
        SMI = 0b010,
        NMI = 0b100,
        INIT = 0b101,
        EXT_INT = 0b111
    };

    enum class destMode : uint8_t
    {
        PHYSICAL = 0,
        LOGICAL = 1
    };

    enum flags
    {
        MASKED = (1 << 0),
        ACTIVE_HIGH_LOW = (1 << 1),
        EDGE_LEVEL = (1 << 3),
    };

    class ioapic
    {
        private:
        uintptr_t mmio_base = 0;
        uint32_t gsi_base = 0;
        size_t redirs = 0;

        constexpr uint32_t entry(uint32_t i)
        {
            return 0x10 + (i * 2);
        }

        uint32_t read(uint32_t reg);
        void write(uint32_t reg, uint32_t value);

        uint64_t read_entry(uint32_t i);
        void write_entry(uint32_t i, uint64_t value);

        public:
        ioapic(uintptr_t phys_mmio_base, uint32_t gsi_base);

        void set(uint8_t i, uint8_t vector, deliveryMode delivery, destMode dest, uint16_t flags, uint8_t id);
        void mask(uint8_t i);
        void unmask(uint8_t i);

        std::pair<uint32_t, uint32_t> gsi_range() const
        {
            return { this->gsi_base, this->gsi_base + this->redirs };
        }
    };

    extern bool initialised;

    void set(uint32_t gsi, uint8_t vector, deliveryMode delivery, destMode dest, uint16_t flags, uint8_t id);
    void mask(uint32_t gsi);
    void unmask(uint32_t gsi);

    void mask_irq(uint8_t irq);
    void unmask_irq(uint8_t irq);

    ioapic *ioapic_for_gsi(uint32_t gsi);

    void init();
} // namespace ioapic