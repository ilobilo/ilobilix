// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <concepts>
#include <cstdint>
#include <cstddef>

namespace rtl8139::spec
{
    enum class registers
    {
        mac0 = 0x00,    // uint8_t
        mac4 = 0x04,    // uint8_t
        mar0 = 0x08,    // uint8_t
        tsd0 = 0x10,    // uint32_t
        tsd1 = 0x14,    // uint32_t
        tsd2 = 0x18,    // uint32_t
        tsd3 = 0x1C,    // uint32_t
        tsad0 = 0x20,   // uint32_t
        tsad1 = 0x24,   // uint32_t
        tsad3 = 0x28,   // uint32_t
        tsad4 = 0x2C,   // uint32_t
        rbstart = 0x30, // uint32_t
        erbcr = 0x34,   // uint16_t
        ersr = 0x36,    // uint8_t
        cmd = 0x37,     // uint8_t
        capr = 0x38,    // uint16_t
        cbr = 0x3A,     // uint16_t
        imr = 0x3C,     // uint16_t
        isr = 0x3E,     // uint16_t
        tcr = 0x40,     // uint32_t
        rcr = 0x44,     // uint32_t
        tctr = 0x48,    // uint32_t
        mpc = 0x4C,     // uint32_t
        cr93c46 = 0x50, // uint8_t
        config0 = 0x51, // uint8_t
        config1 = 0x52, // uint8_t
        msr = 0x58,     // uint8_t
        bmcr = 0x62,    // uint16_t
    };

    constexpr inline auto get_tsd(size_t i)
    {
        return registers(size_t(registers::tsd0) + i * 4);
    }

    constexpr inline auto get_tsad(size_t i)
    {
        return registers(size_t(registers::tsad0) + i * 4);
    }

    template<typename Type>
    concept is_register_offset = (std::unsigned_integral<Type> && sizeof(Type) <= sizeof(uint32_t));

    template<typename Type>
    concept is_register = requires (Type a) {
        is_register_offset<decltype(a.raw)>;
    };

    union rsd
    {
        struct [[gnu::packed]]
        {
            uint16_t rok : 1;
            uint16_t fae : 1;
            uint16_t crc : 1;
            uint16_t lng : 1;
            uint16_t rnt : 1;
            uint16_t ise : 1;
            uint16_t rsv0 : 7;
            uint16_t bar : 1;
            uint16_t pam : 1;
            uint16_t mar : 1;
        };
        uint16_t raw;
    };
    static_assert(sizeof(rsd) == 2);

    union tsd
    {
        struct [[gnu::packed]]
        {
            uint32_t size : 13;
            uint32_t own : 1;
            uint32_t tun : 1;
            uint32_t tok : 1;
            uint32_t ertxth : 6;
            uint32_t rsv0 : 2;
            uint32_t ncc : 4;
            uint32_t cdh : 1;
            uint32_t owc : 1;
            uint32_t tabt : 1;
            uint32_t crs : 1;
        };
        uint32_t raw;
    };
    static_assert(sizeof(tsd) == 4);

    union intmask
    {
        static constexpr auto offset = registers::imr;
        struct [[gnu::packed]]
        {
            uint16_t rok : 1;
            uint16_t rer : 1;
            uint16_t tok : 1;
            uint16_t ter : 1;
            uint16_t rxovw : 1;
            uint16_t lnkch : 1;
            uint16_t fovw : 1;
            uint16_t rsv0 : 6;
            uint16_t lench : 1;
            uint16_t tmout : 1;
            uint16_t syser : 1;
        };
        uint16_t raw;
    };
    static_assert(sizeof(intmask) == 2);

    union intstat
    {
        static constexpr auto offset = registers::isr;
        struct [[gnu::packed]]
        {
            uint16_t rok : 1;
            uint16_t rer : 1;
            uint16_t tok : 1;
            uint16_t ter : 1;
            uint16_t rxovw : 1;
            uint16_t lnkch : 1;
            uint16_t fovw : 1;
            uint16_t rsv0 : 6;
            uint16_t lench : 1;
            uint16_t tmout : 1;
            uint16_t syser : 1;
        };
        uint16_t raw;
    };
    static_assert(sizeof(intstat) == 2);

    union txconf
    {
        static constexpr auto offset = registers::tcr;
        struct [[gnu::packed]]
        {
            uint32_t clrabt : 1;
            uint32_t rsv0 : 3;
            uint32_t txrr : 4;
            uint32_t mxdma : 3;
            uint32_t rsv1 : 5;
            uint32_t crc : 1;
            uint32_t lbk : 2;
            uint32_t rsv2 : 3;
            uint32_t hvidb : 2;
            uint32_t ifg : 2;
            uint32_t hvida : 5;
            uint32_t rsv3 : 1;
        };
        uint32_t raw;
    };
    static_assert(sizeof(txconf) == 4);

    enum class rxbuffer_lengths : uint32_t
    {
        k8p16 = 0b00,
        k16p16 = 0b01,
        k32p16 = 0b10,
        k64p16 = 0b11,
    };

    enum class mxdma_sizes : uint32_t
    {
        b16 = 0b000,
        b32 = 0b001,
        b64 = 0b010,
        b128 = 0b011,
        b256 = 0b100,
        b512 = 0b101,
        b1024 = 0b110,
        unlimited = 0b111,
    };

    union rxconf
    {
        static constexpr auto offset = registers::rcr;
        struct [[gnu::packed]]
        {
            uint32_t aap : 1;
            uint32_t apm : 1;
            uint32_t am : 1;
            uint32_t ab : 1;
            uint32_t ar : 1;
            uint32_t aer : 1;
            uint32_t rsv0 : 1;
            uint32_t wrap : 1;
            uint32_t mxdma : 3;
            uint32_t rblen : 2;
            uint32_t rxfth : 3;
            uint32_t rer8 : 1;
            uint32_t merint : 1;
            uint32_t rsv1 : 6;
            uint32_t erth : 4;
            uint32_t rsv2 : 4;
        };
        uint32_t raw;
    };
    static_assert(sizeof(rxconf) == 4);

    union command
    {
        static constexpr auto offset = registers::cmd;
        struct [[gnu::packed]]
        {
            uint8_t bufe : 1;
            uint8_t rsv0 : 1;
            uint8_t te : 1;
            uint8_t re : 1;
            uint8_t rst : 1;
            uint8_t rsv1 : 3;
        };
        uint8_t raw;
    };
    static_assert(sizeof(command) == 1);

    union cr93c46
    {
        static constexpr auto offset = registers::cr93c46;
        struct [[gnu::packed]]
        {
            uint8_t eedo : 1;
            uint8_t eedi : 1;
            uint8_t eesk : 1;
            uint8_t eecs : 1;
            uint8_t rsv0 : 2;
            uint8_t eemd : 2;
        };
        uint8_t raw;
    };
    static_assert(sizeof(cr93c46) == 1);

    union config0
    {
        static constexpr auto offset = registers::config0;
        struct [[gnu::packed]]
        {
            uint8_t brs : 3;
            uint8_t pl : 2;
            uint8_t t10 : 1;
            uint8_t pcs : 1;
            uint8_t scr : 1;
        };
        uint8_t raw;
    };
    static_assert(sizeof(config0) == 1);

    union config1
    {
        static constexpr auto offset = registers::config1;
        struct [[gnu::packed]]
        {
            uint8_t pmen : 1;
            uint8_t vpd : 1;
            uint8_t iomap : 1;
            uint8_t memmap : 1;
            uint8_t lwact : 1;
            uint8_t dvrload : 1;
            uint8_t leds : 2;
        };
        uint8_t raw;
    };
    static_assert(sizeof(config1) == 1);

    union msr
    {
        static constexpr auto offset = registers::msr;
        struct [[gnu::packed]]
        {
            uint8_t rxpf : 1;
            uint8_t txpf : 1;
            uint8_t lnkb : 1;
            uint8_t sp10 : 1;
            uint8_t auxs : 1;
            uint8_t rsv0 : 1;
            uint8_t rxfc : 1;
            uint8_t txfc : 1;
        };
        uint8_t raw;
    };
    static_assert(sizeof(msr) == 1);

    union bmcr
    {
        static constexpr auto offset = registers::bmcr;
        struct [[gnu::packed]]
        {
            uint16_t rsv0 : 8;
            uint16_t dpx : 1;
            uint16_t ran : 1;
            uint16_t rsv1 : 2;
            uint16_t ane : 1;
            uint16_t spd : 1;
            uint16_t rsv2 : 1;
            uint16_t rst : 1;
        };
        uint16_t raw;
    };
    static_assert(sizeof(bmcr) == 2);
} // namespace rtl8139::spec