// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <arch/x86_64/cpu/idt.hpp>
#include <drivers/pci/pci.hpp>

#include <lib/net/net.hpp>

#include <lib/buffer.hpp>
#include <lib/mmio.hpp>
#include <lib/io.hpp>
#include <expected>

#include "spec.hpp"

namespace rtl8139
{
    inline constexpr uint16_t vendorid = 0x10EC;
    inline constexpr uint16_t deviceid = 0x8139;

    inline constexpr size_t max_packet_size = 0x600;
    inline constexpr size_t min_packet_size = 0x16;

    inline constexpr size_t rx_buffer_size = 8192;
    inline constexpr size_t tx_buffer_size = max_packet_size;

    static constexpr spec::intstat ack
    {
        .rok = 1,
        .rer = 1,
        .tok = 1,
        .ter = 1,
        .rxovw = 1,
        .lnkch = 1,
        .fovw = 1,
        .tmout = 1,
        .syser = 1
    };

    template<typename Type, typename Type1>
    Type1 get_member_type(Type1 Type:: *);
    #define MEMBER_TYPE(x) decltype(get_member_type(& x))

    struct Controller : net::nic
    {
        private:
        pci::device_t *dev;
        uintptr_t base;
        bool is_mmio;

        mem::buffer txs[4];
        mem::buffer rx;

        size_t tx_next;
        size_t rx_offset;

        net::mac::address mac;
        std::mutex lock;

        template<spec::is_register_offset Type>
        Type read(spec::registers reg)
        {
            if (this->is_mmio == true)
                return mmio::in<Type>(this->base + std::to_underlying(reg));
            return io::in<Type>(this->base + std::to_underlying(reg));
        }

        template<spec::is_register_offset Type>
        void write(spec::registers reg, Type value)
        {
            if (this->is_mmio == true)
                mmio::out<Type>(this->base + std::to_underlying(reg), value);
            io::out<Type>(this->base + std::to_underlying(reg), value);
        }

        template<spec::is_register Type> requires (requires (Type a) { { Type::offset }; })
        Type read()
        {
            return { .raw = this->read<MEMBER_TYPE(Type::raw)>(Type::offset) };
        }

        template<spec::is_register Type>
        Type read(spec::registers reg)
        {
            return { .raw =  this->read<MEMBER_TYPE(Type::raw)>(reg) };
        }

        template<spec::is_register Type> requires (requires (Type a) { { Type::offset }; })
        void write(Type obj)
        {
            this->write<decltype(obj.raw)>(Type::offset, obj.raw);
        }

        template<spec::is_register Type>
        void write(spec::registers reg, Type obj)
        {
            this->write<decltype(obj.raw)>(reg, obj.raw);
        }

        void irq_handler();
        void receive();

        public:
        inline net::mac::address mac_address() override
        {
            return this->mac;
        }
        bool send(mem::buffer &&buffer) override;

        Controller(pci::device_t *dev) :
            dev(dev), base(0), is_mmio(false),
            tx_next(0), rx_offset(0), mac() { }

        ~Controller();

        std::expected<void, std::string> init();
    };

    #undef MEMBER_TYPE
} // namespace rtl8139