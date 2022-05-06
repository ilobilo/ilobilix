// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__x86_64__) || defined(_M_X64)

#include <cstddef>
#include <cstdint>

namespace arch::x86_64::gdt
{
    static constexpr uint8_t GDT_NULL = 0x00;
    static constexpr uint8_t GDT_CODE = 0x08;
    static constexpr uint8_t GDT_DATA = 0x10;
    static constexpr uint8_t GDT_USER_CODE = 0x18;
    static constexpr uint8_t GDT_USER_DATA = 0x20;
    static constexpr uint8_t GDT_TSS = 0x28;

    struct [[gnu::packed]] GDTDescriptor
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

    void initcpu(size_t num);
    void init();
} // namespace arch::x86_64::gdt

#endif