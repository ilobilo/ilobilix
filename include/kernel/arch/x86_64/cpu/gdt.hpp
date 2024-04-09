// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <cstddef>
#include <cstdint>

namespace gdt
{
    inline constexpr uint8_t GDT_NULL = 0x00;
    inline constexpr uint8_t GDT_CODE = 0x08;
    inline constexpr uint8_t GDT_DATA = 0x10;
    inline constexpr uint8_t GDT_USER_DATA = 0x18;
    inline constexpr uint8_t GDT_USER_CODE = 0x20;
    inline constexpr uint8_t GDT_TSS = 0x28;

    struct [[gnu::packed]] ptr
    {
        uint16_t size;
        uint64_t offset;
    };

    struct [[gnu::packed]] entry
    {
        uint16_t limit0;
        uint16_t base0;
        uint8_t base1;
        uint8_t access;
        uint8_t granularity;
        uint8_t base2;
    };

    namespace tss
    {
        struct [[gnu::packed]] ptr
        {
            uint32_t reserved0;
            uint64_t rsp[3];
            uint64_t reserved1;
            uint64_t ist[7];
            uint64_t reserved2;
            uint16_t reserved3;
            uint16_t iopboffset;
        };

        struct [[gnu::packed]] entry
        {
            uint16_t length;
            uint16_t base0;
            uint8_t base1;
            uint8_t flags1;
            uint8_t flags2;
            uint8_t base2;
            uint32_t base3;
            uint32_t reserved;
        };
    } // namespace tss

    extern tss::ptr *tsses;

    void init(size_t num);
} // namespace gdt