// Copyright (C) 2022  ilobilo

#include <arch/x86_64/drivers/timers/hpet.hpp>
#include <arch/x86_64/cpu/ioapic.hpp>
#include <arch/x86_64/cpu/idt.hpp>
#include <drivers/pci/pci.hpp>
#include <drivers/smp.hpp>
#include <lib/panic.hpp>
#include <lib/time.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>

namespace timers::hpet
{
    std::vector<comparator*> comparators;
    std::vector<device*> devices;

    bool initialised = false;

    void comparator::start_timer_internal(uint64_t ns)
    {
        this->_device->stop();

        auto &comp = this->_device->regs->comparators[this->_num];
        comp.cmd &= ~((1 << 2) | (1 << 3) | (1 << 6));
        this->_device->regs->ist = (1 << this->_num);

        auto delta = (ns * 1'000'000) / this->_device->clk;
        if (this->_mode == PERIODIC)
        {
            comp.cmd |= (1 << 2) | (1 << 3) | (1 << 6);
            comp.val = this->_device->regs->main_counter + delta;
            comp.val = delta;
        }
        else
        {
            comp.cmd |= (1 << 2);
            comp.val = this->_device->regs->main_counter + delta;
        }

        this->_device->start();
    }

    void comparator::cancel_timer()
    {
        if (this->_int_mode == INT_NONE)
            return;

        lockit(this->lock);

        auto &comp = this->_device->regs->comparators[this->_num];
        comp.cmd &= ~(1 << 2);
        this->_func.clear();
    }

    void device::start()
    {
        this->regs->cmd |= (1 << 0);
    }

    void device::stop()
    {
        this->regs->cmd &= ~(1 << 0);
    }

    device::device(acpi::HPETHeader *table) : regs(reinterpret_cast<HPET*>(tohh(table->address.Address)))
    {
        this->stop();
        this->regs->cmd &= ~0b10;

        this->clk = this->regs->cap >> 32;
        assert(this->clk > 1'000'000 && this->clk <= 0x05F5E100);

        auto rev = static_cast<uint8_t>(this->regs->cap);
        assert(rev != 0);

        this->comp_count = ((this->regs->cap >> 8) & 0x1F) + 1;

        this->regs->main_counter = 0;
        this->regs->ist = this->regs->ist;

        this->_legacy = (this->regs->cap >> 15) & 1;

        log::info("HPET: Found device %d: Legacy replacement mode: %s", table->hpet_number, this->_legacy ? "true" : "false");
        log::info(" Timers:");

        uint32_t gsi_mask = 0xFFFFFFFF;
        for (size_t i = 0; i < this->comp_count; i++)
        {
            auto &timer = this->comps[i];
            timer._fsb = (this->regs->comparators[i].cmd >> 15) & 1;
            timer._periodic = (this->regs->comparators[i].cmd >> 4) & 1;
            timer._int_route = this->regs->comparators[i].cmd >> 32;

            log::info("  - Comparator %d: FSB: %s, Periodic: %s", i, timer._fsb ? "true" : "false", timer._periodic ? "true" : "false");

            gsi_mask &= timer._int_route;
        }

        uint32_t gsi = 0xFFFFFFFF;
        uint8_t gsi_vector = 0;

        for (size_t i = 0; i < this->comp_count; i++)
        {
            auto &timer = this->comps[i];
            timer._int_mode = timer.INT_NONE;
            timer._device = this;
            timer._num = i;

            this->regs->comparators[i].cmd &= ~((1 << 14) | (0x1F << 9) | (1 << 8) | (1 << 6) | (1 << 3) | (1 << 2) | (1 << 1));

            if (timer._fsb == true)
            {
                auto [handler, vector] = idt::allocate_handler();
                handler.set([this, &timer](cpu::registers_t *regs)
                {
                    lockit(timer.lock);

                    if (timer._func == false)
                        return;

                    timer._func(regs);

                    if (timer._mode != PERIODIC)
                    {
                        this->regs->comparators[timer._num].cmd &= ~(1 << 2);
                        timer._func.clear();
                    }
                });
                timer._vector = vector;

                pci::msi::address addr
                {
                    .base_address = 0xFEE,
                    .destination_id = static_cast<uint8_t>(this_cpu()->arch_id)
                };

                pci::msi::data data
                {
                    .delivery_mode = 0x00,
                    .vector = vector
                };

                this->regs->comparators[i].fsb = (static_cast<uint64_t>(addr.raw) << 32) | data.raw;
                this->regs->comparators[i].cmd |= (1 << 14);
                timer._int_mode = timer.INT_FSB;
            }
            else if (ioapic::initialised == true && __builtin_popcount(gsi_mask) <= 24) // TODO: Fix this on VBOX
            {
                if (gsi == 0xFFFFFFFF)
                {
                    for (ssize_t g = 31; g >= 0; g--)
                    {
                        if (gsi_mask & (1 << g))
                        {
                            if (ioapic::ioapic_for_gsi(g) == nullptr)
                                continue;
                            gsi = g;
                            break;
                        }
                    }
                    assert(gsi != 0xFFFFFFFF);

                    gsi_vector = gsi + 0x20;
                    idt::handlers[gsi_vector].set([this](cpu::registers_t *regs)
                    {
                        for (size_t i = 0; i < this->comp_count; i++)
                        {
                            if (this->regs->ist & (1 << i))
                            {
                                auto &timer = this->comps[i];
                                lockit(timer.lock);

                                if (timer._func == false)
                                    return;

                                timer._func(regs);

                                if (timer._mode != PERIODIC)
                                {
                                    this->regs->comparators[timer._num].cmd &= ~(1 << 2);
                                    timer._func.clear();
                                }

                                this->regs->ist = (1 << i);
                            }
                        }
                    });

                    ioapic::set(gsi, gsi_vector, ioapic::deliveryMode::FIXED, ioapic::destMode::PHYSICAL, ioapic::EDGE_LEVEL, this_cpu()->arch_id);
                }

                this->regs->comparators[i].cmd |= ((gsi & 0x1F) << 9) | (1 << 1);
                assert(((this->regs->comparators[i].cmd >> 9) & 0x1F) == gsi);

                timer._vector = gsi_vector;
                timer._int_mode = timer.INT_IOAPIC;
            }
            else
            {
                log::error("HPET: Neither standard nor FSB interrupt mappings are supported!");
                continue;
            }

            comparators.push_back(&this->comps[i]);
        }

        this->start();
    }

    void device::nsleep(uint64_t ns)
    {
        uint64_t target = this->regs->main_counter + ((ns * 1'000'000) / this->clk);

        while(this->regs->main_counter < target)
            asm volatile ("pause");
    }

    void device::msleep(uint64_t ms)
    {
        uint64_t target = this->regs->main_counter + ((ms * 1'000'000'000'000) / this->clk);

        while(this->regs->main_counter < target)
            asm volatile ("pause");
    }

    uint64_t device::time_ns()
    {
        return (this->regs->main_counter * this->clk) / 1'000'000;
    }

    uint64_t device::time_ms()
    {
        return (this->regs->main_counter * this->clk) / 1'000'000'000'000;
    }

    void nsleep(uint64_t ns)
    {
        devices.front()->nsleep(ns);
    }

    void msleep(uint64_t ms)
    {
        devices.front()->msleep(ms);
    }

    uint64_t time_ns()
    {
        return devices.front()->time_ns();
    }

    uint64_t time_ms()
    {
        return devices.front()->time_ms();
    }

    void cancel_timer(comparator *comp)
    {
        lockit(lock);
        comp->cancel_timer();
    }

    void init()
    {
        log::info("Initialising HPET...");

        auto table = acpi::findtable<acpi::HPETHeader>("HPET", 0);

        if (table == nullptr)
        {
            log::error("HPET table not found!");
            return;
        }

        for (size_t i = 0; table != nullptr; i++, table = acpi::findtable<acpi::HPETHeader>("HPET", i))
            devices.push_back(new device(table));

        initialised = true;
    }
} // namespace timers::hpet