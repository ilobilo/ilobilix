// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <drivers/pci/pci.hpp>

#include <lib/net/net.hpp>
#include <lib/buffer.hpp>
#include <lib/mmio.hpp>
#include <lib/io.hpp>
#include <expected>

#include "spec.hpp"

namespace e1000
{
    inline constexpr uint16_t vendorid = 0x8086;
    inline constexpr uint16_t deviceids[] {
        0x100E, // E1000 QEMU, Bochs, VBox
        0x153A, // Intel I217
        0x10EA  // Intel 82577LM
    };

    inline constexpr size_t rx_desc_num = 32;
    inline constexpr size_t tx_desc_num = 32;

    inline constexpr size_t rx_desc_size = rx_desc_num * sizeof(spec::rxd);
    inline constexpr size_t tx_desc_size = tx_desc_num * sizeof(spec::txd);

    inline constexpr auto default_bsize = spec::bsizes::b8192;

    struct Controller : net::nic
    {
        private:
        pci::device_t *dev;
        uintptr_t base;
        bool is_mmio;
        bool eeprom;

        mem::buffer<spec::rxd> rxs;
        mem::buffer<spec::txd> txs;

        size_t cur_rx;
        size_t cur_tx;

        net::mac::address mac;
        std::mutex lock;

        uint16_t eeprom_read(uint8_t _addr)
        {
            auto addr = static_cast<uint32_t>(_addr);
            uint32_t tmp = 0;
            if (this->eeprom == true)
            {
                this->write(spec::registers::eprm, 1 | (addr << 8));
                while (!((tmp = this->read(spec::registers::eprm)) & (1 << 4)))
                    arch::pause();
            }
            else
            {
                this->write(spec::registers::eprm, 1 | (addr << 2));
                while (!((tmp = this->read(spec::registers::eprm)) & (1 << 1)))
                    arch::pause();
            }
            return static_cast<uint16_t>((tmp >> 16) & 0xFFFF);
        }

        uint32_t read(spec::registers reg)
        {
#if CAN_LEGACY_IO
            if (this->is_mmio == false)
            {
                io::out<uint32_t>(this->base, std::to_underlying(reg));
                return io::in<uint32_t>(this->base + sizeof(uint32_t));
            }
#endif
            return mmio::in<uint32_t>(this->base + std::to_underlying(reg));
        }

        void write(spec::registers reg, uint32_t value)
        {
#if CAN_LEGACY_IO
            if (this->is_mmio == false)
            {
                io::out<uint32_t>(this->base, std::to_underlying(reg));
                io::out<uint32_t>(this->base + sizeof(uint32_t), value);
                return;
            }
#endif
            mmio::out<uint32_t>(this->base + std::to_underlying(reg), value);
        }

        template<spec::is_register Type> requires (requires (Type a) { { Type::offset }; })
        Type read()
        {
            return { .raw = this->read(Type::offset) };
        }

        template<spec::is_register Type>
        Type read(spec::registers reg)
        {
            return { .raw = this->read(reg) };
        }

        template<spec::is_register Type> requires (requires (Type a) { { Type::offset }; })
        void write(Type obj)
        {
            this->write(Type::offset, obj.raw);
        }

        template<spec::is_register Type>
        void write(spec::registers reg, Type obj)
        {
            this->write(reg, obj.raw);
        }

        void irq_handler();
        void receive();

        bool detect_eeprom();

        void reset();
        void link_up();

        public:
        inline net::mac::address mac_address() override
        {
            return this->mac;
        }
        bool send(mem::u8buffer &&buffer) override;

        Controller(pci::device_t *dev) :
            dev(dev), base(0),
            is_mmio(false), eeprom(false),
            cur_rx(0), cur_tx(0), mac() { }

        ~Controller();

        std::expected<void, std::string> init();
    };
} // namespace e1000