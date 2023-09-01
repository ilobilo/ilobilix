// Copyright (C) 2022-2023  ilobilo

#include <arch/x86_64/cpu/ioapic.hpp>
#include <arch/x86_64/cpu/idt.hpp>
#include <arch/x86_64/cpu/pic.hpp>
#include <drivers/acpi.hpp>
#include <init/kernel.hpp>
#include <lib/panic.hpp>
#include <lib/mmio.hpp>
#include <lib/log.hpp>
#include <mm/vmm.hpp>

namespace ioapic
{
    static std::vector<ioapic> ioapics;
    bool initialised = false;

    uint32_t ioapic::read(uint32_t reg)
    {
        mmio::out<uint32_t>(this->mmio_base, reg);
        return mmio::in<uint32_t>(this->mmio_base + 0x10);
    }

    void ioapic::write(uint32_t reg, uint32_t value)
    {
        mmio::out<uint32_t>(this->mmio_base, reg);
        mmio::out<uint32_t>(this->mmio_base + 0x10, value);
    }

    uint64_t ioapic::read_entry(uint32_t i)
    {
        return this->read(this->entry(i)) | (static_cast<uint64_t>(this->read(this->entry(i) + 0x01)) << 32);
    }

    void ioapic::write_entry(uint32_t i, uint64_t value)
    {
        this->write(this->entry(i), static_cast<uint32_t>(value));
        this->write(this->entry(i) + 0x01, static_cast<uint32_t>(value >> 32));
    }

    ioapic::ioapic(uintptr_t phys_mmio_base, uint32_t gsi_base) : gsi_base(gsi_base)
    {
        this->mmio_base = tohh(phys_mmio_base);
        // this->mmio_base = malloc<uintptr_t>(0x1000);
        // vmm::kernel_pagemap->map(this->mmio_base, phys_mmio_base, vmm::rw);

        this->redirs = ((this->read(0x01) >> 16) & 0xFF) + 1;

        for (size_t i = 0; i < this->redirs; i++)
            this->mask(i);
    }

    void ioapic::set(uint8_t i, uint8_t vector, delivery delivery, destmode dest, uint16_t flags, uint8_t id)
    {
        uint64_t value = 0;
        value |= vector;
        value |= std::to_underlying(delivery) << 8;
        value |= std::to_underlying(dest) << 11;

        if (flags & active_low)
            value |= (1 << 13);
        if (flags & level_sensative)
            value |= (1 << 15);
        if (flags & masked)
            value |= (1 << 16);

        value |= static_cast<uint64_t>(id) << 56;

        this->write_entry(i, value);
    }

    void ioapic::mask(uint8_t i)
    {
        this->write_entry(i, this->read_entry(i) | (1 << 16));
    }

    void ioapic::unmask(uint8_t i)
    {
        this->write_entry(i, this->read_entry(i) & ~(1 << 16));
    }

    ioapic *ioapic_for_gsi(uint32_t gsi)
    {
        for (auto &entry : ioapics)
        {
            auto [start, end] = entry.gsi_range();
            if (start <= gsi && end >= gsi)
                return &entry;
        }

        return nullptr;
    }

    std::optional<std::pair<uint8_t, uint16_t>> irq_for_gsi(uint32_t gsi)
    {
        for (const auto &entry : acpi::isos)
        {
            if (entry.gsi == gsi)
                return std::make_pair(entry.irq_source, entry.flags);
        }
        return std::nullopt;
    }

    static ioapic &internal_ioapic_for_gsi(uint32_t gsi)
    {
        auto ret = ioapic_for_gsi(gsi);
        if (ret == nullptr)
            PANIC("Couldn't find IOAPIC for GSI {}", gsi);

        return *ret;
    }

    void set(uint32_t gsi, uint8_t vector, delivery delivery, destmode dest, uint16_t flags, uint8_t id)
    {
        auto &entry = internal_ioapic_for_gsi(gsi);
        entry.set(gsi - entry.gsi_range().first, vector, delivery, dest, flags, id);
    }

    void mask(uint32_t gsi)
    {
        auto &entry = internal_ioapic_for_gsi(gsi);
        entry.mask(gsi - entry.gsi_range().first);
    }

    void unmask(uint32_t gsi)
    {
        auto &entry = internal_ioapic_for_gsi(gsi);
        entry.unmask(gsi - entry.gsi_range().first);
    }

    void mask_irq(uint8_t irq)
    {
        for (const auto &iso : acpi::isos)
        {
            if (iso.irq_source == irq)
            {
                mask(iso.gsi);
                return;
            }
        }
        mask(irq);
    }

    void unmask_irq(uint8_t irq)
    {
        for (const auto &iso : acpi::isos)
        {
            if (iso.irq_source == irq)
            {
                unmask(iso.gsi);
                return;
            }
        }
        unmask(irq);
    }

    void init()
    {
        log::infoln("IOAPIC: Initialising...");

        if (acpi::madthdr == nullptr || acpi::ioapics.empty())
        {
            // TODO: We currently don't support legacy PIC
            // log::errorln("MADT table not found!");
            PANIC("MADT table not found!");
            return;
        }

        pic::disable();

        for (const auto &entry : acpi::ioapics)
            ioapics.emplace_back(entry.addr, entry.gsib);

        auto redirect_isa_irq = [](size_t i)
        {
            for (const auto &iso : acpi::isos)
            {
                if (iso.irq_source == i)
                {
                    set(iso.gsi, iso.irq_source + 0x20, delivery::fixed, destmode::physical, iso.flags | masked, smp_request.response->bsp_lapic_id);
                    idt::handlers[iso.irq_source + 0x20].reserve();
                    return;
                }
            }

            set(i, i + 0x20, delivery::fixed, destmode::physical, masked, smp_request.response->bsp_lapic_id);
            idt::handlers[i + 0x20].reserve();
        };

        if (acpi::madthdr->legacy_pic())
        {
            for (size_t i = 0; i < 16; i++)
            {
                if (i == 2)
                    continue;
                redirect_isa_irq(i);
            }
        }

        initialised = true;
    }
} // namespace ioapic