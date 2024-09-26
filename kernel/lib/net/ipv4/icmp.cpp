// Copyright (C) 2022-2024  ilobilo

#include <lib/net/ipv4/icmp.hpp>

namespace net::ipv4::icmp
{
    void processor::push_packet(mem::u8buffer &&buffer, ipv4::frame &&frame)
    {
        auto icmp = frame::from_ipv4(frame);
        if (icmp.type == types::echo_request)
        {
            if (frame.dip != this->_sender->ipv4())
                return;

            auto response = build_packet(
                ethernet::frame { this->_sender->mac(), frame.ether.source, ethernet::types::ipv4, nullptr, 0 },
                ipv4::frame { this->_sender->ipv4(), frame.sip, std::to_underlying(ipv4::protocol::icmp), 64, 0, 5, nullptr, 0, { } },
                icmp::frame { icmp::types::echo_reply, 0, 0, icmp.ident, icmp.seq, icmp.payload, icmp.payload_size }
            );
            this->_sender->send(std::move(response));
        }
    }

    bool processor::matches(const ipv4::frame &frame)
    {
        return frame.proto == std::to_underlying(ipv4::protocol::icmp);
    }
} // namespace net::ipv4::icmp