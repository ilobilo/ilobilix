// Copyright (C) 2022  ilobilo

#include <drivers/acpi/acpi.hpp>
#include <lai/helpers/sci.h>
#include <cpu/apic/apic.hpp>
#include <cpu/idt/idt.hpp>
#include <cpu/pic/pic.hpp>
#include <lib/panic.hpp>
#include <lib/timer.hpp>
#include <lib/mmio.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>
#include <main.hpp>
#include <cpuid.h>

namespace apic
{
    bool initialised = false;
    static bool x2apic = false;

    namespace lapic
    {
        static uint32_t reg2x2apic(uint32_t reg)
        {
            uint32_t x2apic_reg = 0;
            if (reg == 0x310) x2apic_reg = 0x30;
            else x2apic_reg = reg >> 4;
            return x2apic_reg + 0x800;
        }

        static void set_nmi(uint8_t vec, uint8_t current_processor_id, uint8_t processor_id, uint16_t flags, uint8_t lint)
        {
            if (processor_id != 0xFF) if (current_processor_id != processor_id) return;

            uint32_t nmi = 0x400 | vec;

            if (flags & 2) nmi |= 1 << 13;
            if (flags & 8) nmi |= 1 << 15;
            if (lint == 0) lapic::write(0x350, nmi);
            else if (lint == 1) lapic::write(0x360, nmi);
        }

        uint32_t read(uint32_t reg)
        {
            if (x2apic) return cpu::rdmsr(reg2x2apic(reg));
            return mmin<uint32_t>(reinterpret_cast<void*>(acpi::lapic_addr + reg));
        }

        void write(uint32_t reg, uint32_t value)
        {
            if (x2apic) cpu::wrmsr(reg2x2apic(reg), value);
            else mmout<uint32_t>(reinterpret_cast<void*>(acpi::lapic_addr + reg), value);
        }

        void init(uint8_t processor_id)
        {
            uint64_t apic_msr = cpu::rdmsr(0x1B) | (1 << 11);
            uint32_t a = 0, b = 0, c = 0, d = 0;
            if (__get_cpuid(1, &a, &b, &c, &d))
            {
                if (c & (1 << 21))
                {
                    x2apic = true;
                    apic_msr |= 1 << 10;
                }
            }

            cpu::wrmsr(0x1B, apic_msr);
            lapic::write(0x80, 0x00);
            lapic::write(0xF0, lapic::read(0xF0) | 0x100);

            if (x2apic == false)
            {
                lapic::write(0xE0, 0xF0000000);
                lapic::write(0xD0, lapic::read(0x20));
            }

            for (auto nmi : acpi::nmis)
            {
                set_nmi(2, processor_id, nmi->processor, nmi->flags, nmi->lint);
            }
        }
    } // namespace lapic

    namespace ioapic
    {
        static uint32_t get_gsi_count(uintptr_t ioapic_address)
        {
            return (ioapic::read(ioapic_address, 1) & 0xFF0000) >> 16;
        }

        static acpi::MADTIOApic *get_ioapic_by_gsi(uint32_t gsi)
        {
            for (auto ioapic : acpi::ioapics)
            {
                if (ioapic->gsib <= gsi && ioapic->gsib + get_gsi_count(ioapic->addr) > gsi) return ioapic;
            }
            return nullptr;
        }

        uint32_t read(uintptr_t address, size_t reg)
        {
            mmout<uint32_t>(reinterpret_cast<void*>(address), reg & 0xFF);
            return mmin<uint32_t>(reinterpret_cast<void*>(address + 16));
        }

        void write(uintptr_t address, size_t reg, uint32_t data)
        {
            mmout<uint32_t>(reinterpret_cast<void*>(address), reg & 0xFF);
            mmout<uint32_t>(reinterpret_cast<void*>(address + 16), data);
        }

        void redirect_gsi(uint32_t gsi, uint8_t vec, uint16_t flags)
        {
            size_t io_apic = get_ioapic_by_gsi(gsi)->addr;

            uint32_t low_index = 0x10 + (gsi - get_ioapic_by_gsi(gsi)->gsib) * 2;
            uint32_t high_index = low_index + 1;

            uint32_t high = ioapic::read(io_apic, high_index);

            high &= ~0xFF000000;
            high |= read(io_apic, 0) << 24;
            ioapic::write(io_apic, high_index, high);

            uint32_t low = ioapic::read(io_apic, low_index);

            low &= ~(1 << 16);
            low &= ~(1 << 11);
            low &= ~0x700;
            low &= ~0xFF;
            low |= vec;
            if (flags & 2) low |= 1 << 13;
            if (flags & 8) low |= 1 << 15;

            ioapic::write(io_apic, low_index, low);
        }

        void redirect_irq(uint32_t irq, uint8_t vect)
        {
            for (auto iso : acpi::isos)
            {
                if (iso->irq_source == irq)
                {
                    redirect_gsi(iso->gsi, vect, iso->flags);
                    return;
                }
            }

            redirect_gsi(irq, vect, 0);
        }
    } // namespace ioapic

    void send_ipi(uint32_t lapic_id, uint32_t flags)
    {
        if (x2apic) cpu::wrmsr(0x830, (static_cast<uint64_t>(lapic_id) << 32) | flags);
        else
        {
            lapic::write(0x310, (lapic_id << 24));
            lapic::write(0x300, flags);
        }
    }

    void eoi()
    {
        lapic::write(0xB0, 0x00);
    }

    static void sci_handler(cpu::registers_t *)
    {
        uint16_t event = lai_get_sci_event();
        if (event & ACPI_POWER_BUTTON)
        {
            acpi::shutdown();
            timer::msleep(50);
            outw(0xB004, 0x2000);
            outw(0x604, 0x2000);
            outw(0x4004, 0x3400);
        }
    }

    void init()
    {
        log::info("Initialising APIC...");

        if (acpi::madt == false || acpi::madthdr == nullptr)
        {
            log::error("Could not find MADT table!\n");
            return;
        }

        pic::disable();
        lapic::init(acpi::lapics[0]->processor_id);

        uint8_t vector = acpi::fadthdr->SCI_Interrupt + 32;
        idt::handlers[vector].set(sci_handler);
        idt::handlers[vector].ioapic_redirect(vector);

        initialised = true;
    }
} // namespace apic