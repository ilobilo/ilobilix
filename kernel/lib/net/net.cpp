// Copyright (C) 2022-2023  ilobilo

#include <lib/net/ipv4/icmp.hpp>
#include <lib/net/arp.hpp>

#include <frg/manual_box.hpp>
#include <vector>

#include <drivers/proc.hpp>
#include <init/kernel.hpp>

namespace net
{
    struct entry
    {
        std::unique_ptr<nic> device;
        ethernet::dispatcher<
                ipv4::processor<
                    ipv4::icmp::processor
                >,
                arp::processor
            > dispatcher;

        entry(std::unique_ptr<nic> dev) :
            device(std::move(dev)), dispatcher(device.get())
        { device->attach_receiver(&this->dispatcher); }
    };
    std::vector<frg::manual_box<entry>> devices;

    static uint8_t ip_last_num = 69;
    void register_nic(std::unique_ptr<nic> device)
    {
        // Because std::unique_ptr and veque::veque couldn't get along very well
        auto &ref = devices.emplace_back();
        ref.initialize(std::move(device));

        ref->dispatcher.set_ipv4({ 192, 168, 1, ip_last_num++ });
        proc::enqueue(new proc::thread(kernel_proc, ref->dispatcher.runner, &ref->dispatcher));
    }
} // namespace net