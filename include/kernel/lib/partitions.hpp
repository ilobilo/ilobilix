// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <string_view>
#include <cstdint>

namespace mbr
{
    struct [[gnu::packed]] partition
    {
        struct [[gnu::packed]] chs
        {
            uint8_t head;
            uint8_t sector : 6;
            uint16_t cylinder : 10;
        };

        uint8_t flags;
        chs chs_start;
        uint8_t sid;
        chs chs_last;
        uint32_t lba_start;
        uint32_t lba_count;

        inline constexpr bool is_bootable()
        {
            return this->flags == 0x80;
        }

        inline constexpr bool is_pmbr()
        {
            return this->flags == 0xEE;
        }

        inline constexpr bool is_unused()
        {
            return this->sid == 0;
        }
    };

    struct table
    {
        uint8_t bootstrap[446];
        partition partitions[4];
        uint8_t signature[2];

        inline constexpr bool is_read_only()
        {
            return this->bootstrap[444] == 0x5A && this->bootstrap[445] == 0x5A;
        }

        inline constexpr bool is_valid()
        {
            return this->signature[0] == 0x55 && this->signature[1] == 0xAA;
        }
    };
    static_assert(sizeof(table) == 512);
} // namespace mbr

namespace gpt
{
    struct partition
    {
        uint128_t partguid;
        uint128_t upartguid;
        uint64_t lbastart;
        uint64_t lbaend;
        uint64_t attributes;
        char16_t name[36];

        inline constexpr bool is_unused()
        {
            return this->partguid == 0;
        }

        inline constexpr bool is_system()
        {
            return this->attributes & 1;
        }

        inline constexpr bool is_bootpart()
        {
            return this->attributes & 2;
        }

        inline constexpr std::u16string_view get_name()
        {
            return this->name;
        }
    };
    static_assert(sizeof(partition) == 128);

    constexpr char signature[] { 'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T' };
    struct [[gnu::packed]] table
    {
        char8_t signature[8];
        uint32_t revision;
        uint32_t header_size;
        uint32_t hdrchecksum;
        uint32_t reserved;
        uint64_t lba;
        uint64_t altlba;
        uint64_t firstblock;
        uint64_t lastblock;
        uint128_t guid;
        uint64_t guidpartlba;
        uint32_t parts;
        uint32_t partentrysize;
        uint32_t partchecksum;

        inline constexpr bool is_valid()
        {
            return std::u8string_view(this->signature) == signature;
        }
    };
    static_assert(sizeof(table) == 92);
} // namespace gpt