// Copyright (C) 2022-2024  ilobilo

#include <arch/arch.hpp>

#include <lib/log.hpp>
#include <lib/io.hpp>

#include <module.hpp>

#include "ps2.hpp"

namespace ps2
{
    namespace kbd { void init(uint8_t irq); }
    namespace ports
    {
        // TODO: WTH doesn't it work with 0
        static uint16_t data = 0x1234;
        static uint16_t command = 0x1234;
        static uint16_t &status = command;
    } // namespace ports

    enum config : uint8_t
    {
        first_irq_mask = (1 << 0),
        second_irq_mask = (1 << 1),
        second_clock = (1 << 5),
        translation = (1 << 6)
    };

    uint8_t read()
    {
        while ((io::in<uint8_t>(ports::status) & (1 << 0)) == 0)
            arch::pause();
        return io::in<uint8_t>(ports::data);
    }

    void write(uint16_t port, uint8_t data)
    {
        while ((io::in<uint8_t>(ports::status) & (1 << 1)) != 0)
            arch::pause();
        io::out<uint8_t>(port, data);
    }

    void flush()
    {
        while ((io::in<uint8_t>(ports::status) & (1 << 0)) != 0)
            io::in<uint8_t>(ports::data);
    }

    // Used in x86_64/arch.cpp for PS2 reboot
    extern "C" uint16_t ps2_command_port;

    bool probe(uacpi_namespace_node *node, uacpi_namespace_node_info *info)
    {
        log::infoln("PS2: Initialising...");

        uint8_t irq = 1;
        {
            uacpi_resources *res;
            uacpi_status st = uacpi_get_current_resources(node, &res);
            if (uacpi_unlikely_error(st))
            {
                log::errorln("PS2: Failed to retrieve resources: {}", uacpi_status_to_string(st));
                return false;
            }

            uacpi_for_each_resource(res, [](void *data, uacpi_resource *resource)
            {
                auto set_ports = [&](uint16_t val)
                {
                    if (ports::data == 0x1234)
                        ports::data = val;
                    else if (ports::command == 0x1234)
                        ports::command = val;
                };

                switch (resource->type)
                {
                    case UACPI_RESOURCE_TYPE_FIXED_IO:
                        set_ports(resource->fixed_io.address);
                        break;
                    case UACPI_RESOURCE_TYPE_IO:
                        set_ports(resource->io.minimum);
                        break;
                    case UACPI_RESOURCE_TYPE_IRQ:
                        assert(resource->irq.num_irqs == 1);
                        *static_cast<uint8_t *>(data) = resource->irq.irqs[0];
                        break;
                }
                return UACPI_RESOURCE_ITERATION_CONTINUE;
            }, &irq);

            uacpi_free_resources(res);
            if (ports::data == 0 || ports::command == 0)
            {
                log::errorln("PS2: Failed to retrieve ports");
                return false;
            }

            ps2_command_port = ports::command;
        }
        {
            write(ports::command, 0xAD);
            write(ports::command, 0xA7);

            write(ports::command, 0x20);
            auto config = read();

            config |= first_irq_mask | translation;
            if (config & second_clock)
                config |= second_irq_mask;

            write(ports::command, 0x60);
            write(ports::data, config);

            write(ports::command, 0xAE);
            if (config & second_clock)
                write(ports::command, 0xA8);

            kbd::init(irq);
            log::infoln("PS2: Keyboard initialised");

            flush();
        }

        return true;
    }
} // namespace ps2

ACPI_DRIVER(ps2, ps2::probe, "PNP0303")