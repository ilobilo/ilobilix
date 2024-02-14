// Copyright (C) 2022-2024  ilobilo

#include <arch/arch.hpp>
#include <lib/mmio.hpp>
#include <mm/vmm.hpp>

namespace serial
{
    static constexpr uintptr_t uart = 0x9000000;
    static uintptr_t addr = -1;

    void printc(char c)
    {
        if (addr == uintptr_t(-1)) [[unlikely]]
            return;

        while (mmio::in<uint16_t>(addr + 0x18) & (1 << 5))
            arch::pause();

        mmio::out<uint8_t>(addr, c);
    }

    char readc()
    {
        if (addr == uintptr_t(-1)) [[unlikely]]
            return 0;

        while (mmio::in<uint16_t>(addr + 0x18) & (1 << 4))
            arch::pause();

        return mmio::in<uint8_t>(addr);
    }

    // Can't use UART if 0x9000000 is not mapped
    void early_init() { }

    void second_early_init()
    {
        uintptr_t vaddr = vmm::alloc_vspace(vmm::vsptypes::other, 0x1000);
        vmm::kernel_pagemap->map(vaddr, uart, vmm::rw, vmm::mmio);
        addr = vaddr;

        // Disable the UART.
        mmio::out<uint16_t>(addr + 0x30, 0);

        // Set word length to 8 bits and enable FIFOs
        mmio::out<uint16_t>(addr + 0x2C, (3 << 5) | (1 << 4));

        // Enable UART, TX and RX
        mmio::out<uint16_t>(addr + 0x30, (1 << 0) | (1 << 8) | (1 << 9));
    }

    void init()
    {
    }
} // namespace serial