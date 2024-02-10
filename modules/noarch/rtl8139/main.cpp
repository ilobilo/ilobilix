// Copyright (C) 2022-2023  ilobilo

#include <drivers/pci/pci.hpp>
#include <drivers/smp.hpp>

#include <lib/panic.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <module.hpp>

#include "rtl8139.hpp"

namespace rtl8139
{
    void Controller::irq_handler()
    {
        auto status = this->read<spec::intstat>();
        this->write(ack);

        if (status.rok == 1)
            this->receive();
        if (status.rer == 1)
            PANIC("RTL8139: RX error not implemented!");
        if (status.tok == 1) { /* Transmit OK */ }
        if (status.ter == 1)
            PANIC("RTL8139: TX error not implemented!");
        if (status.rxovw == 1)
        {
            // TODO: ???
            /* RX buffer overflow */
            this->write<uint16_t>(spec::registers::mpc, 0);
            this->rx_offset = this->read<uint16_t>(spec::registers::cbr) % rx_buffer_size;
            this->write<uint16_t>(spec::registers::capr, this->rx_offset - 0x10);
        }
        if (status.lnkch == 1) { /* Link change */ }
        if (status.fovw == 1)
            PANIC("RTL8139: RX FIFO overflow not implemented!");
        if (status.lench == 1) { /* Cable length change */ }
        if (status.tmout == 1)
            PANIC("RTL8139: Timeout not implemented!");
        if (status.syser == 1)
            PANIC("RTL8139: System error not implemented!");
    }

    void Controller::receive()
    {
        std::unique_lock guard(this->lock);

        auto packet_ptr = this->rx.virt_data() + this->rx_offset;
        auto status = reinterpret_cast<spec::rsd*>(packet_ptr);
        auto length = *reinterpret_cast<uint16_t*>(packet_ptr + 2);

        auto packet_ok = [&] {
            bool badpacket = status->rnt ||
                             status->lng ||
                             status->crc ||
                             status->fae;

            if (badpacket == false && status->rok)
            {
                if (length < min_packet_size || length > max_packet_size)
                    return false;
                return true;
            }
            return false;
        };

        if (packet_ok() == false)
        {
            auto cmd = this->read<spec::command>();
            cmd.te = 1;
            cmd.re = 1;
            this->write(cmd);
            return;
        }

        mem::u8buffer buffer(packet_ptr + 4, length - 4);

        uint16_t nic_rx_offset = (uint16_t(this->rx_offset + length + 4 + 3) & ~3);
        this->write<uint16_t>(spec::registers::capr, nic_rx_offset - 0x10);
        this->rx_offset = nic_rx_offset % rx_buffer_size;

        // log::infoln("RTL8139: Packet received");

        if (this->receiver != nullptr)
            this->receiver->receive(std::move(buffer));
    }

    bool Controller::send(mem::u8buffer &&buffer)
    {
        if (buffer.size() > max_packet_size)
            return false;

        std::unique_lock guard(this->lock);

        size_t buffer_index = 0;
        for (size_t i = 0; i < 4; i++)
        {
            auto potential_buffer = (this->tx_next + i) % 4;
            auto status = this->read<spec::tsd>(spec::get_tsd(potential_buffer));
            if (status.own == 1)
            {
                buffer_index = potential_buffer;
                goto skip;
            }
        }
        return false;

        skip:
        auto &transmit = this->txs[buffer_index];
        this->tx_next = (buffer_index + 1) % 4;

        memcpy(transmit.virt_data(), buffer.virt_data(), buffer.size());
        if (auto left = tx_buffer_size - buffer.size(); left > 0)
            memset(transmit.virt_data() + buffer.size(), 0, left);

        auto tsd = this->read<spec::tsd>(spec::get_tsd(buffer_index));
        // tsd.size = std::max(buffer.size(), 60zu);
        tsd.size = buffer.size();
        tsd.own = 0;
        this->write(spec::get_tsd(buffer_index), tsd);

        return true;
    }

    using expected_void = std::expected<void, std::string>;
    expected_void Controller::init()
    {
        this->dev->command(pci::CMD_BUS_MAST, true);

        for (auto &bar : this->dev->bars)
        {
            if (bar.type == pci::PCI_BARTYPE_MMIO)
            {
                this->is_mmio = true;
                this->base = bar.map();
                break;
            }
#if CAN_LEGACY_IO
            else if (bar.type == pci::PCI_BARTYPE_IO)
                this->base = bar.base;
#endif
        }

        if (this->is_mmio == true)
            this->dev->command(pci::CMD_MEM_SPACE, true);
#if CAN_LEGACY_IO
        else this->dev->command(pci::CMD_IO_SPACE, true);
#else
        else return std::unexpected("Legacy IO is not supported on this architecture");
#endif

        for (auto &buffer : this->txs)
            buffer.allocate(tx_buffer_size);
        this->rx.allocate(rx_buffer_size + 16 + 1500); // 1500 for WRAP bit

        auto cmd = this->read<spec::command>();
        cmd.rst = 1;
        this->write(cmd);

        log::infoln("RTL8139: Resetting...");
        while (this->read<spec::command>().rst == 1)
            arch::pause();

        auto cr93c46 = this->read<spec::cr93c46>();
        cr93c46.eemd = 0b11;
        this->write(cr93c46);

        auto cfg1 = this->read<spec::config1>();
        cfg1.lwact = 0;
        this->write(cfg1);

        cmd = this->read<spec::command>();
        cmd.te = 1;
        cmd.re = 1;
        this->write(cmd);

        this->write<uint32_t>(spec::registers::rbstart, reinterpret_cast<uintptr_t>(this->rx.phys_data()));
        this->write<uint8_t>(spec::registers::mpc, 0);

        auto bmcr = this->read<spec::bmcr>();
        bmcr.spd = 1;
        bmcr.dpx = 1;
        bmcr.ane = 1;
        this->write(bmcr);

        auto msr = this->read<spec::msr>();
        msr.rxfc = 1;
        this->write(msr);

        auto rxconf = this->read<spec::rxconf>();
        rxconf.aap = 1;
        rxconf.apm = 1;
        rxconf.am = 1;
        rxconf.ab = 1;
        rxconf.wrap = 1;
        rxconf.mxdma = std::to_underlying(spec::mxdma_sizes::b1024);
        rxconf.rblen = std::to_underlying(spec::rxbuffer_lengths::k8p16);
        rxconf.rxfth = 0b111;
        this->write(rxconf);

        auto txconf = this->read<spec::txconf>();
        txconf.txrr = 0;
        txconf.mxdma = std::to_underlying(spec::mxdma_sizes::b1024);
        txconf.ifg = 0b11;
        this->write(txconf);

        for (size_t i = 0; auto &buffer : this->txs)
            this->write<uint32_t>(spec::get_tsad(i++), reinterpret_cast<uintptr_t>(buffer.phys_data()));

        cr93c46 = this->read<spec::cr93c46>();
        cr93c46.eemd = 0b00;
        this->write(cr93c46);

        cmd = this->read<spec::command>();
        cmd.te = 1;
        cmd.re = 1;
        this->write(cmd);

        // TODO: eeprom
        auto mac0 = this->read<uint32_t>(spec::registers::mac0);
        auto mac4 = this->read<uint32_t>(spec::registers::mac4);

        this->mac[0] = mac0 & 0xFF;
        this->mac[1] = (mac0 >> 8) & 0xFF;
        this->mac[2] = (mac0 >> 16) & 0xFF;
        this->mac[3] = (mac0 >> 24) & 0xFF;
        this->mac[4] = mac4 & 0xFF;
        this->mac[5] = (mac4 >> 8) & 0xFF;

        auto imr = this->read<spec::intmask>();
        imr.rok = 1;
        imr.rer = 1;
        imr.tok = 1;
        imr.ter = 1;
        imr.rxovw = 1;
        imr.lnkch = 1;
        imr.fovw = 1;
        imr.tmout = 1;
        imr.syser = 1;
        this->write(imr);

        if (this->dev->register_irq(smp::bsp_id, [&] { this->irq_handler(); }) == false)
            return std::unexpected("Could not install interrupt handler");

        this->write(ack);
        this->dev->command(pci::CMD_INT_DIS, false);

        log::infoln("RTL8139: Initialisation complete. MAC address: {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
            this->mac[0], this->mac[1], this->mac[2], this->mac[3], this->mac[4], this->mac[5]);

        return expected_void();
    }

    Controller::~Controller()
    {
        this->dev->unregister_irq();
    }
} // namespace rtl8139

DRIVER(rtl8139, init, fini)

__init__ bool init()
{
    bool at_least_one = false;
    for (const auto dev : pci::get_devices())
    {
        if (dev->vendorid != rtl8139::vendorid || dev->deviceid != rtl8139::deviceid)
            continue;

        log::infoln("RTL8139: Found controller");

        auto ctrl = std::make_unique<rtl8139::Controller>(dev);
        auto ret = ctrl->init();
        if (ret.has_value())
        {
            at_least_one = true;
            net::register_nic(std::move(ctrl));
        }
        else log::errorln("RTL8139: {}!", ret.error());
    }

    return at_least_one;
}

__fini__ bool fini()
{
    assert(false, "RTL8139->fini() not implemented!");
    return false;
}