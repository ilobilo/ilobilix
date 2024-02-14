// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <initializer_list>
#include <cassert>
#include <cstdint>
#include <cstddef>

namespace net
{
    namespace ipv4
    {
        struct address
        {
            uint8_t addr[4];

            constexpr address() = default;
            constexpr address(std::initializer_list<uint8_t> ilist)
            {
                assert(ilist.size() == 4);
                this->from_bytes(ilist.begin());
            }

            constexpr address(const uint8_t *data)
            {
                this->from_bytes(data);
            }

            constexpr void from_bytes(const uint8_t *bytes)
            {
                this->addr[0] = bytes[0];
                this->addr[1] = bytes[1];
                this->addr[2] = bytes[2];
                this->addr[3] = bytes[3];
            }

            constexpr uint8_t *to_bytes(uint8_t *ptr)
            {
                for (auto i : this->addr)
                    *ptr++ = i;
                return ptr;
            }

            constexpr size_t size() { return 4; }

            constexpr uint8_t &operator[](size_t i)
            {
                return this->addr[i];
            }

            constexpr bool operator==(const address &other) const
            {
                return this->addr[0] == other.addr[0] &&
                       this->addr[1] == other.addr[1] &&
                       this->addr[2] == other.addr[2] &&
                       this->addr[3] == other.addr[3];
            }

            constexpr operator bool() const
            {
                return !this->operator==({ 0, 0, 0, 0 });
            }
        };
        inline constexpr address broadcast { 255, 255, 255, 255 };
    } // namespace ipv4

    namespace mac
    {
        struct address
        {
            uint8_t addr[6];

            constexpr address() = default;
            constexpr address(std::initializer_list<uint8_t> ilist)
            {
                assert(ilist.size() == 6);
                this->from_bytes(ilist.begin());
            }

            constexpr address(const uint8_t *data)
            {
                this->from_bytes(data);
            }

            constexpr void from_bytes(const uint8_t *bytes)
            {
                this->addr[0] = bytes[0];
                this->addr[1] = bytes[1];
                this->addr[2] = bytes[2];
                this->addr[3] = bytes[3];
                this->addr[4] = bytes[4];
                this->addr[5] = bytes[5];
            }

            constexpr uint8_t *to_bytes(uint8_t *ptr)
            {
                for (auto i : this->addr)
                    *ptr++ = i;
                return ptr;
            }

            constexpr size_t size() { return 6; }

            constexpr uint8_t &operator[](size_t i)
            {
                return this->addr[i];
            }

            constexpr bool operator==(const address &other) const
            {
                return this->addr[0] == other.addr[0] &&
                       this->addr[1] == other.addr[1] &&
                       this->addr[2] == other.addr[2] &&
                       this->addr[3] == other.addr[3] &&
                       this->addr[4] == other.addr[4] &&
                       this->addr[5] == other.addr[5];
            }

            constexpr operator bool() const
            {
                return !this->operator==({ 0, 0, 0, 0, 0, 0 });
            }
        };
        inline constexpr address broadcast { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    } // namespace mac
} // namespace net