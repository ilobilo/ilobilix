// Copyright (C) 2024-2025  ilobilo

module;

#include <uacpi/tables.h>
#include <uacpi/acpi.h>

module x86_64.drivers.timers.hpet;

import system.memory;
import system.time;
import boot;
import arch;
import lib;
import cppstd;

namespace x86_64::timers::hpet
{
    namespace regs
    {
        constexpr std::size_t cap = 0x00;
        constexpr std::size_t cfg = 0x10;
        constexpr std::size_t cnt = 0xF0;
    } // namespace regs
    namespace
    {
        std::uintptr_t paddr;
        std::uintptr_t vaddr;
        bool is_64bit;

        std::int64_t offset = 0;
        std::uint64_t p, n;

        std::uint64_t read(std::size_t offset)
        {
            return lib::mmio::in<64>(vaddr + offset);
        }

        void write(std::size_t offset, std::uint64_t value)
        {
            lib::mmio::out<64>(vaddr + offset, value);
        }

        // TODO: 64 bit overflows? :trl:
        std::uint64_t read()
        {
            constexpr std::uint64_t mask = 0xFFFFFFFFul;
            static std::atomic<std::uint64_t> last = 0;

            auto value = read(regs::cnt);
            if (is_64bit)
                return value;

            auto last_val = last.load(std::memory_order_relaxed);
            value &= mask;
            value |= last_val & ~mask;

            if (value < last_val)
                value += (1ul << 32);

            if (last_val - value > (mask >> 1))
                last.compare_exchange_strong(last_val, value, std::memory_order_relaxed, std::memory_order_relaxed);

            return value;
        }
    } // namespace

    bool supported()
    {
        static const auto cached = [] -> bool
        {
            uacpi_table table;
            if (uacpi_table_find_by_signature(ACPI_HPET_SIGNATURE, &table) != UACPI_STATUS_OK)
                return false;

            const auto hpet = static_cast<acpi_hpet *>(table.ptr);
            if (hpet->address.address_space_id != UACPI_ADDRESS_SPACE_SYSTEM_MEMORY)
            {
                uacpi_table_unref(&table);
                return false;
            }
            paddr = hpet->address.address;

            uacpi_table_unref(&table);
            return true;
        } ();
        return cached;
    }

    std::uint64_t time_ns()
    {
        lib::ensure(!!initialised);
        return lib::ticks2ns(read(), p, n) - offset;
    }

    void calibrate(std::size_t ms)
    {
        const auto ticks = (ms * frequency) / 1'000;

        const auto start = read();
        auto current = start;

        while (current < start + ticks)
            current = read();
    }

    time::clock clock { "hpet", 25, time_ns };
    void init()
    {
        log::info("hpet: supported: {}", supported());

        vaddr = vmm::alloc_vpages(vmm::space_type::other, 1);
        log::debug("hpet: mapping to 0x{:X}", vaddr);

        const auto psize = vmm::page_size::small;
        const auto npsize = vmm::pagemap::from_page_size(psize);

        if (!vmm::kernel_pagemap->map(vaddr, paddr, npsize, vmm::flag::rw, psize, vmm::caching::mmio))
            lib::panic("could not map HPET");

        const auto cap = read(regs::cap);
        is_64bit = (cap & ACPI_HPET_COUNT_SIZE_CAP);

        frequency = 1'000'000'000'000'000ull / (cap >> 32);
        std::tie(p, n) = lib::freq2nspn(frequency);

        log::debug("hpet: timer is {} bit, frequency: {} hz", is_64bit ? "64" : "32", frequency);

        // enable main counter
        write(regs::cfg, 1);

        initialised = true;
        if (const auto clock = time::main_clock())
            offset = time_ns() - clock->ns();

        time::register_clock(clock);
    }
} // namespace x86_64::timers::hpet