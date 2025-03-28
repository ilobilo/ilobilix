// Copyright (C) 2022-2024  ilobilo

module x86_64.system.gdt;

import system.cpu.self;
import system.memory;
import boot;
import lib;
import std;

namespace x86_64::gdt
{
    namespace
    {
        [[gnu::naked]] void load(std::uintptr_t gdtr, std::uint8_t data, std::uint8_t code, std::uint16_t tss)
        {
            asm volatile (R"(
                lgdt [rdi]

                mov ss, si
                mov si, 0
                mov ds, si
                mov fs, si
                mov gs, si
                mov es, si

                push rdx
                lea rax, [rip + 1f]
                push rax
                retfq
                1:
                ltr cx
                ret
            )");
        }
    } // namespace

    void init_on(cpu::processor *cpu)
    {
        if (cpu->idx == cpu::bsp_idx)
            log::info("gdt: loading on BSP");

        auto &gdt = cpu->arch.gdt;
        auto &tss = cpu->arch.tss;

        auto allocate_stack = [] {
            auto stack = lib::tohh(pmm::alloc(boot::kernel_stack_size / pmm::page_size));
            std::memset(stack, 0, boot::kernel_stack_size);
            return reinterpret_cast<std::uintptr_t>(stack) + boot::kernel_stack_size;
        };
        tss.rsp[0] = allocate_stack(); // cpl3 to cpl0
        tss.ist[0] = allocate_stack(); // page fault
        tss.ist[1] = allocate_stack(); // scheduler

        tss.iopboffset = sizeof(tss);

        const auto base = reinterpret_cast<std::uintptr_t>(&tss);
        const std::uint16_t limit = sizeof(tss::ptr) - 1;

        gdt = entries {
            { 0x0000, 0x0000, 0x00, 0b00000000, 0x0, 0b0000, 0x00 }, // null
            { 0x0000, 0x0000, 0x00, 0b10011010, 0x0, 0b0010, 0x00 }, // kernel code
            { 0x0000, 0x0000, 0x00, 0b10010010, 0x0, 0b0010, 0x00 }, // kernel data
            { 0x0000, 0x0000, 0x00, 0b11111010, 0x0, 0b0100, 0x00 }, // user code 32 bit
            { 0x0000, 0x0000, 0x00, 0b11110010, 0x0, 0b0010, 0x00 }, // user data
            { 0x0000, 0x0000, 0x00, 0b11111010, 0x0, 0b0010, 0x00 }, // user code
            { // tss
                limit,
                static_cast<std::uint16_t>(base),
                static_cast<std::uint8_t>(base >> 16),
                0b10001001, 0x0, 0x0,
                static_cast<std::uint8_t>(base >> 24),
                static_cast<std::uint32_t>(base >> 32),
                0x00000000
            }
        };

        const ptr gdtr {
            sizeof(entries) - 1,
            reinterpret_cast<std::uintptr_t>(&gdt),
        };

        load(reinterpret_cast<std::uintptr_t>(&gdtr), segment::data, segment::code, segment::tss);
    }
} // namespace x86_64::gdt