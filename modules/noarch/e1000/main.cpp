// Copyright (C) 2022-2024  ilobilo

#include <drivers/smp.hpp>

#include <lib/containers.hpp>
#include <lib/panic.hpp>
#include <lib/log.hpp>

#include <module.hpp>

#include "e1000.hpp"

namespace e1000
{
    void Controller::irq_handler()
    {
        auto cause = this->read(spec::registers::icause);
        this->write(spec::registers::icause, cause);

        // TODO
        if (cause & 0x01) { /* Transmit OK */ }
        if (cause & 0x04)
            this->link_up();
        else if (cause & 0x80)
            this->receive();
    }

    void Controller::receive()
    {
        std::unique_lock guard(this->lock);

        while (true)
        {
            auto idx = this->cur_rx;
            auto &desc = this->rxs.at(idx);
            if ((desc.status & 0x01) != 0x01)
                break;

            mem::u8buffer buffer(reinterpret_cast<uint8_t*>(desc.addr), desc.length);

            desc.status = 0;
            this->cur_rx = (idx + 1) % rx_desc_num;
            this->write(spec::registers::rxdtl, idx);

            // log::infoln("E1000: Packet received");

            if (this->receiver != nullptr)
                this->receiver->receive(std::move(buffer));
        }
    }

    bool Controller::send(mem::u8buffer &&buffer)
    {
        std::unique_lock guard(this->lock);

        auto &desc = this->txs.at(this->cur_tx);
        desc.addr = reinterpret_cast<uint64_t>(buffer.phys_data());
        desc.length = buffer.size_bytes();
        desc.cmd = 0b1011;
        desc.status = 0;

        this->cur_tx = (this->cur_tx + 1) % tx_desc_num;
        this->write(spec::registers::txdtl, this->cur_tx);

        while (!(desc.status & 0xFF))
            arch::pause();

        return true;
    }

    bool Controller::detect_eeprom()
    {
        this->write(spec::registers::eprm, 1);
        for (size_t i = 0; i < 1000; i++)
        {
            if (this->read(spec::registers::eprm) & (1 << 4))
                return this->eeprom = true;
        }
        return this->eeprom = false;
    }

    void Controller::reset()
    {
        auto ctrl = this->read<spec::control>();
        ctrl.rst = 1;
        this->write(ctrl);

        while ((ctrl = this->read<spec::control>()).rst == 1)
            arch::pause();

        ctrl.lnkrs = 0;
        ctrl.phy_rst = 0;
        ctrl.vme = 0;
        this->write(ctrl);
    }

    void Controller::link_up()
    {
        auto ctrl = this->read<spec::control>();
        ctrl.slu = 1;
        this->write(ctrl);

        while (this->read<spec::status>().lnkup == 0)
            arch::pause();
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

        log::infoln("E1000: Resetting...");
        this->reset();

        log::infoln("E1000: EEPROM available: {}", this->detect_eeprom());
        {
            uint32_t mac0 = 0;
            uint16_t mac4 = 0;

            if (this->eeprom == true)
            {
                mac0 = (this->eeprom_read(0) | (static_cast<uint32_t>(this->eeprom_read(1)) << 16));
                mac4 = this->eeprom_read(2);

            }
            else if (this->is_mmio == true)
            {
                mac0 = this->read(spec::registers::mac0);
                mac4 = this->read(spec::registers::mac4);
            }
            else return std::unexpected("Could not get MAC address. Neither EEPROM nor MMIO is available");

            this->mac[0] = mac0 & 0xFF;
            this->mac[1] = (mac0 >> 8) & 0xFF;
            this->mac[2] = (mac0 >> 16) & 0xFF;
            this->mac[3] = (mac0 >> 24) & 0xFF;
            this->mac[4] = mac4 & 0xFF;
            this->mac[5] = (mac4 >> 8) & 0xFF;
        }

        this->rxs.allocate(rx_desc_num);
        {
            for (auto &desc : this->rxs)
            {
                new (&desc) spec::rxd {
                    .addr = pmm::alloc<uint64_t>(div_roundup(spec::bsize2size(default_bsize), pmm::page_size)),
                    .length = 0,
                    .chksum = 0,
                    .status = 0,
                    .error = 0,
                    .spec = 0
                };
            }

            auto phys = reinterpret_cast<uint64_t>(this->rxs.phys_data());
            this->write(spec::registers::rxdlow, static_cast<uint32_t>(phys));
            this->write(spec::registers::rxdhi, static_cast<uint32_t>(phys >> 32));

            this->write(spec::registers::rxdlen, rx_desc_size);
            this->write(spec::registers::rxdhd, 0);
            this->write(spec::registers::rxdtl, rx_desc_num - 1);

            this->write(spec::rxcontrol {
                .en = 1,
                .upe = 1,
                .lpe = 1,
                .mpe = 1,
                .lbm = 0,
                .rsv1 = 2, // 0: 1/2, 1: 1/4, 2: 1/8, might not affect anything
                .rsv2 = 0,
                .bam = 1,
                .secrc = 1
            }.set_bsize(default_bsize));
        }

        this->txs.allocate(tx_desc_num);
        {
            for (auto &desc : this->txs)
            {
                new (&desc) spec::txd {
                    .addr = 0,
                    .length = 0,
                    .cso = 0,
                    .cmd = 0,
                    .status = std::to_underlying(spec::tstats::descdone),
                    .css = 0,
                    .spec = 0
                };
            }

            auto phys = reinterpret_cast<uint64_t>(this->txs.phys_data());
            this->write(spec::registers::txdlow, static_cast<uint32_t>(phys));
            this->write(spec::registers::txdhi, static_cast<uint32_t>(phys >> 32));

            this->write(spec::registers::txdlen, tx_desc_size);
            this->write(spec::registers::txdhd, 0);
            this->write(spec::registers::txdtl, 0);

            // I hope this is correct
            this->write(spec::txcontrol {
                .en = 1,
                .psp = 1,
                .ct = 15,
                .bst = 64,
                .rtlc = 1,
                .rsv5 = 0b0011,
            });

            // IPGT: 10, IPGR1: 8, IPGR: 6 ?????????????
            // this->write(spec::registers::txipg, 0x0060200A);

            // This is what spec recommends
            this->write(spec::txipg {
                .ipgt = 8,
                .ipgr1 = ((8 + 4) / 3) * 2,
                .ipgr = 7
            });
        }

        for (size_t i = 0; i < 128; i++)
            this->write(static_cast<spec::registers>(std::to_underlying(spec::registers::mta0) + i * sizeof(uint32_t)), 0);

        if (this->dev->register_irq(smp::bsp_id, [&] { this->irq_handler(); }) == false)
            return std::unexpected("Could not install interrupt handler");

        this->write(spec::registers::imaskset,
            spec::intflags::txdw   |
            // spec::intflags::txqe   |
            spec::intflags::lsc    |
            // spec::intflags::rxdmt0 |
            // spec::intflags::dsw    |
            // spec::intflags::rxo    |
            spec::intflags::rxt0   |
            // spec::intflags::mdac   |
            // spec::intflags::phyint |
            // spec::intflags::lsecpn |
            // spec::intflags::txdl   |
            // spec::intflags::srpd   |
            // spec::intflags::ack    |
            // spec::intflags::eccer
            0
        );
        this->read(spec::registers::icause);

        this->link_up();

        log::infoln("E1000: MAC address: {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
            this->mac[0], this->mac[1], this->mac[2], this->mac[3], this->mac[4], this->mac[5]);

        return expected_void();
    }

    Controller::~Controller()
    {
        this->dev->unregister_irq();

        for (auto &desc : this->rxs)
            pmm::free(desc.addr, div_roundup(spec::bsize2size(default_bsize), pmm::page_size));
    }
} // namespace e1000

GENERIC_DRIVER(e1000, init, fini)

__init__ bool init()
{
    // TODO: Fix interrupt spam
    return false;

    bool at_least_one = false;
    for (const auto dev : pci::get_devices())
    {
        if (dev->vendorid != e1000::vendorid || !contains(e1000::deviceids, dev->deviceid))
            continue;

        log::infoln("E1000: Found controller");

        auto ctrl = std::make_unique<e1000::Controller>(dev);
        auto ret = ctrl->init();
        if (ret.has_value())
        {
            at_least_one = true;
            net::register_nic(std::move(ctrl));
        }
        else log::errorln("E1000: {}", ret.error());
    }

    return at_least_one;
}

__fini__ bool fini()
{
    assert(false, "E1000->fini() not implemented");
    return false;
}