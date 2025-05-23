// Copyright (C) 2024-2025  ilobilo

module drivers.serial;

import system.memory;
import magic_enum;
import arch;
import lib;
import cppstd;

namespace serial
{
    namespace
    {
        constexpr std::uintptr_t uart = 0x9000000;
        std::uintptr_t addr = -1;
    } // namespace

    void printc(char c)
    {
        if (addr == static_cast<std::uintptr_t>(-1)) [[unlikely]]
            return;

        while (lib::mmio::in<16>(addr + 0x18) & (1 << 5))
            arch::pause();

        lib::mmio::out<8>(addr, c);
    }

    void early_init() { }

    void init()
    {
        if (const auto ret = vmm::kernel_pagemap->map(addr = uart, uart, pmm::page_size, vmm::flag::rw, vmm::page_size::small, vmm::caching::mmio); !ret)
            lib::panic("could not map uart: {}", magic_enum::enum_name(ret.error()));

        // Disable the UART.
        lib::mmio::out<16>(addr + 0x30, 0);

        // Set word length to 8 bits and enable FIFOs
        lib::mmio::out<16>(addr + 0x2C, (3 << 5) | (1 << 4));

        // Enable UART, TX and RX
        lib::mmio::out<16>(addr + 0x30, (1 << 0) | (1 << 8) | (1 << 9));
    }
} // namespace serial