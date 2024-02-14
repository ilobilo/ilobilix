// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <concepts>
#include <cstdint>
#include <cstddef>
#include <utility>

namespace e1000::spec
{
    enum class registers : uint32_t
    {
        ctrl = 0x0000,
        stts = 0x0008,
        eprm = 0x0014,

        icause = 0x00C0,
        imask = 0x00D0,

        rctrl = 0x0100,
        tctrl = 0x0400,
        txipg = 0x0410,

        rxdlow = 0x2800,
        rxdhi = 0x2804,
        rxdlen = 0x2808,
        rxdhd = 0x2810,
        rxdtl = 0x2818,

        txdlow = 0x3800,
        txdhi = 0x3804,
        txdlen = 0x3808,
        txdhd = 0x3810,
        txdtl = 0x3818,

        mta0 = 0x5200,

        mac0 = 0x5400,
        mac4 = 0x5404,
    };

    template<typename Type>
    concept is_register_offset = (std::unsigned_integral<Type> && sizeof(Type) <= sizeof(uint32_t));

    template<typename Type>
    concept is_register = requires (Type a) {
        is_register_offset<decltype(a.raw)>;
    };

    enum class speeds : uint32_t
    {
        mbs10 = 0b00,
        mbs100 = 0b01,
        mbs1000 = 0b10,
        mbs1000_ = 0b11,
    };

    enum class tstats : uint8_t
    {
        descdone = 1 << 0,
        excscols = 1 << 1,
        latecols = 1 << 2,
        txundrun = 1 << 3,
    };

    enum intflags : uint32_t
    {
        txdw = 1 << 0,
        txqe = 1 << 1,
        lsc = 1 << 2,
        rxdmt0 = 1 << 4,
        dsw = 1 << 5,
        rxo = 1 << 6,
        rxt0 = 1 << 7,
        mdac = 1 << 9,
        phyint = 1 << 12,
        lsecpn = 1 << 14,
        txdl = 1 << 15,
        srpd = 1 << 16,
        ack = 1 << 17,
        eccer = 1 << 22
    };

    enum class bsizes : uint32_t
    {
        none = ~static_cast<uint32_t>((0b11 << 16) | (1 << 25)),
        b16384 = (0b01 << 16) | (1 << 25),
        b8192 = (0b10 << 16) | (1 << 25),
        b4096 = (0b11 << 16) | (1 << 25),
        b2048 = 0b00 << 16,
        b1024 = 0b01 << 16,
        b512 = 0b10 << 16,
        b256 = 0b11 << 16
    };

    inline constexpr size_t bsize2size(bsizes bsize)
    {
        switch (bsize)
        {
            case bsizes::b16384:
                return 16384;
            case bsizes::b8192:
                return 8192;
            case bsizes::b4096:
                return 4096;
            case bsizes::b2048:
                return 2048;
            case bsizes::b1024:
                return 1024;
            case bsizes::b512:
                return 512;
            case bsizes::b256:
                return 256;
            case bsizes::none:
                return 0;
        }
        std::unreachable();
    }

    union control
    {
        static constexpr auto offset = registers::ctrl;
        struct [[gnu::packed]]
        {
            uint32_t fd : 1;
            uint32_t rsv0 : 1;
            uint32_t giomd : 1;
            uint32_t lnkrs : 1;
            uint32_t rsv1 : 1;
            uint32_t rsv2 : 1; // must be 0, was asde
            uint32_t slu : 1;
            uint32_t ilos : 1;
            uint32_t speed : 2;
            uint32_t rsv3 : 1;
            uint32_t frcspd : 1;
            uint32_t frcdpx : 1;
            uint32_t rsv4 : 3;
            uint32_t sdp0_gpen : 1;
            uint32_t sdp1_gpen : 1;
            uint32_t sdp0_data : 1;
            uint32_t sdp1_data : 1;
            uint32_t advd3wuc : 1;
            uint32_t sdp0_wde : 1;
            uint32_t sdp0_iodr : 1;
            uint32_t sdp1_iodr : 1;
            uint32_t rsv5 : 2;
            uint32_t rst : 1;
            uint32_t rfce : 1;
            uint32_t tfce : 1;
            uint32_t rsv6 : 1;
            uint32_t vme : 1;
            uint32_t phy_rst : 1;
        };
        uint32_t raw;
    };
    static_assert(sizeof(control) == 4);

    union status
    {
        static constexpr auto offset = registers::stts;
        struct [[gnu::packed]]
        {
            uint32_t fd : 1;
            uint32_t lnkup : 1;
            uint32_t lanid : 2;
            uint32_t txoff : 1;
            uint32_t rsv0 : 1;
            uint32_t speed : 2;
            uint32_t asdv : 2;
            uint32_t phyra : 1;
            uint32_t rsv1 : 3;
            uint32_t nvfs : 4;
            uint32_t iovmd : 1;
            uint32_t giome : 1;
            uint32_t rsv2 : 11;
            uint32_t dmaen : 1;
        };
        uint32_t raw;
    };
    static_assert(sizeof(status) == 4);

    union rxcontrol
    {
        static constexpr auto offset = registers::rctrl;
        struct [[gnu::packed]]
        {
            uint32_t rsv0 : 1;
            uint32_t en : 1;
            uint32_t sbp : 1;
            uint32_t upe : 1;
            uint32_t mpe : 1;
            uint32_t lpe : 1;
            uint32_t lbm : 2;
            uint32_t rsv1 : 2;
            uint32_t rsv2 : 2;
            uint32_t mo : 2;
            uint32_t rsv3 : 1;
            uint32_t bam : 1;
            uint32_t bsize : 2;
            uint32_t vfe : 1;
            uint32_t cfien : 1;
            uint32_t cfi : 1;
            uint32_t psp : 1;
            uint32_t dpf : 1;
            uint32_t pmcf : 1;
            uint32_t rsv4 : 2;
            uint32_t secrc : 1;
            uint32_t rsv5 : 5;
        };
        uint32_t raw;

        constexpr rxcontrol &set_bsize(bsizes bsize)
        {
            this->raw &= std::to_underlying(bsizes::none);
            this->raw |= std::to_underlying(bsize);
            return *this;
        }
    };
    static_assert(sizeof(rxcontrol) == 4);

    union txcontrol
    {
        static constexpr auto offset = registers::tctrl;
        struct [[gnu::packed]]
        {
            uint32_t rsv0 : 1;
            uint32_t en : 1;
            uint32_t rsv1 : 1;
            uint32_t psp : 1;
            uint32_t ct : 8;
            uint32_t bst : 10;
            uint32_t swxoff : 1;
            uint32_t rsv2 : 1;
            uint32_t rtlc : 1;
            uint32_t rsv3 : 1;
            uint32_t rsv4 : 2;
            uint32_t rsv5 : 4;
        };
        uint32_t raw;
    };
    static_assert(sizeof(txcontrol) == 4);

    union txipg
    {
        static constexpr auto offset = registers::txipg;
        struct [[gnu::packed]]
        {
            uint32_t ipgt : 10;
            uint32_t ipgr1 : 10;
            uint32_t ipgr : 10;
            uint32_t rsv0 : 2;
        };
        uint32_t raw;
    };
    static_assert(sizeof(txipg) == 4);

    struct [[gnu::packed]] rxd
    {
        uint64_t addr;
        uint16_t length;
        uint16_t chksum;
        uint8_t status;
        uint8_t error;
        uint16_t spec;
    };
    static_assert(sizeof(rxd) == 16);

    struct [[gnu::packed]] txd
    {
        uint64_t addr;
        uint16_t length;
        uint8_t cso;
        uint8_t cmd;
        uint8_t status;
        uint8_t css;
        uint16_t spec;
    };
    static_assert(sizeof(txd) == 16);
} // namespace e1000::spec