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

    struct [[gnu::packed]] GDTR
    {
        uint16_t Size;
        uint64_t Offset;
    };

    struct [[gnu::packed]] GDTEntry
    {
        uint16_t Limit0;
        uint16_t Base0;
        uint8_t Base1;
        uint8_t Access;
        uint8_t Granularity;
        uint8_t Base2;
    };

    struct [[gnu::packed]] TSSEntry
    {
        uint16_t Length;
        uint16_t Base0;
        uint8_t Base1;
        uint8_t Flags1;
        uint8_t Flags2;
        uint8_t Base2;
        uint32_t Base3;
        uint32_t Reserved;
    };

    struct [[gnu::packed]] GDT
    {
        GDTEntry Null;
        GDTEntry Code;
        GDTEntry Data;
        GDTEntry UserCode;
        GDTEntry UserData;
        TSSEntry Tss;
    };

    struct [[gnu::packed]] TSS
    {
        uint32_t Reserved0;
        uint64_t RSP[3];
        uint64_t Reserved1;
        uint64_t IST[7];
        uint64_t Reserved2;
        uint16_t Reserved3;
        uint16_t IOPBOffset;
    };

    extern TSS *tss;

    void init(size_t num);
} // namespace gdt