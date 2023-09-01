// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <optional>
#include <utility>
#include <cstddef>
#include <cstdint>

namespace ioapic
{
    enum class delivery : uint8_t
    {
        fixed = 0b000,
        low_priority = 0b001,
        smi = 0b010,
        nmi = 0b100,
        init = 0b101,
        ext_int = 0b111
    };

    enum class destmode : uint8_t
    {
        physical = 0,
        logical = 1
    };

    enum flags
    {
        masked = (1 << 0),
        active_low = (1 << 1),
        level_sensative = (1 << 3),
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

        void set(uint8_t i, uint8_t vector, delivery delivery, destmode dest, uint16_t flags, uint8_t id);
        void mask(uint8_t i);
        void unmask(uint8_t i);

        std::pair<uint32_t, uint32_t> gsi_range() const
        {
            return { this->gsi_base, this->gsi_base + this->redirs };
        }
    };

    extern bool initialised;

    void set(uint32_t gsi, uint8_t vector, delivery delivery, destmode dest, uint16_t flags, uint8_t id);
    void mask(uint32_t gsi);
    void unmask(uint32_t gsi);

    void mask_irq(uint8_t irq);
    void unmask_irq(uint8_t irq);

    ioapic *ioapic_for_gsi(uint32_t gsi);
    std::optional<std::pair<uint8_t, uint16_t>> irq_for_gsi(uint32_t gsi);

    void init();
} // namespace ioapic