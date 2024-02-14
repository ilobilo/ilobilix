// Copyright (C) 2022-2024  ilobilo

#include <lib/net/arp.hpp>

namespace net::arp
{
    void processor::submit_query(ipv4::address dip)
    {
        auto buffer = build_packet(
            ethernet::frame { this->_sender->mac(), mac::broadcast, ethernet::types::arp, nullptr, 0 },
            arp::frame::query_for(this->_sender->mac(), this->_sender->ipv4(), dip)
        );
        this->_sender->send(std::move(buffer));
    }

    void processor::submit_reply(mac::address dmac, ipv4::address dip)
    {
        auto buffer = build_packet(
            ethernet::frame { this->_sender->mac(), dmac, ethernet::types::arp, nullptr, 0 },
            arp::frame::reply_for(this->_sender->mac(), this->_sender->ipv4(), dmac, dip)
        );
        this->_sender->send(std::move(buffer));
    }

    void processor::push_packet(mem::u8buffer &&, ethernet::frame &&frame)
    {
        auto arp = arp::frame::from_ethernet(frame);
        switch (arp.opcode)
        {
            case opcodes::arp_request:
                if (arp.dip == this->_sender->ipv4())
                    this->submit_reply(arp.smac, arp.sip);
                break;
            case opcodes::arp_reply:
                for (auto &route : this->routes)
                {
                    if (route->ip != arp.sip)
                        continue;

                    route->ip = arp.sip;
                    route->mac = arp.smac;
                    route->resolved = true;
                    route->doorbell.trigger();
                }
                break;
            default:
                break;
        }
    }

    mac::address processor::mac_of(ipv4::address ipv4)
    {
        for (auto &route : this->routes)
        {
            if (route->ip != ipv4)
                continue;

            route->doorbell.await();
            assert(route->resolved == true);
            return route->mac;
        }

        auto &route = this->routes.emplace_back(new arp::route(ipv4, mac::address(), false));
        this->submit_query(ipv4);

        route->doorbell.await();
        assert(route->resolved == true);
        return route->mac;
    }

    bool processor::matches(const ethernet::frame &frame)
    {
        return frame.type == ethernet::types::arp;
    }
} // namespace net::arp