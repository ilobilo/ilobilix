// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <lib/net/ethernet.hpp>
#include <vector>

namespace net::arp
{
    enum hwtypes : uint16_t
    {
        ethernet = 0x0001
    };
    enum protypes : uint16_t
    {
        ipv4 = 0x0800
    };
    enum opcodes : uint16_t
    {
        arp_request = 1,
        arp_reply = 2,
        rarp_request = 3,
        rarp_reply = 4,
    };

    struct frame
    {
        uint16_t hwtype;
        uint16_t protype;
        uint8_t hwsize;
        uint8_t prosize;
        uint16_t opcode;

        mac::address smac;
        ipv4::address sip;
        mac::address dmac;
        ipv4::address dip;

        static constexpr frame from_ethernet(const ethernet::frame &data)
        {
            const uint8_t *bytes = data.payload;
            frame ret;
            ret.hwtype = (uint16_t(bytes[0]) << 8) | bytes[1];
            ret.protype = (uint16_t(bytes[2]) << 8) | bytes[3];

            ret.hwsize = bytes[4];
            ret.prosize = bytes[5];
            ret.opcode = (uint16_t(bytes[6]) << 8) | bytes[7];

            assert(ret.hwtype == hwtypes::ethernet);
            assert(ret.protype == protypes::ipv4);
            assert(ret.hwsize == 6);
            assert(ret.prosize == 4);

            ret.smac.from_bytes(bytes + 8);
            ret.sip.from_bytes(bytes + 14);
            ret.dmac.from_bytes(bytes + 18);
            ret.dip.from_bytes(bytes + 24);

            return ret;
        }

        constexpr uint8_t *to_bytes(uint8_t *ptr)
        {
            assert(this->hwtype == hwtypes::ethernet);
            assert(this->protype == protypes::ipv4);
            assert(this->hwsize == 6);
            assert(this->prosize == 4);

            *ptr++ = (this->hwtype >> 8) & 0xFF;
            *ptr++ = this->hwtype & 0xFF;
            *ptr++ = (this->protype >> 8) & 0xFF;
            *ptr++ = this->protype & 0xFF;
            *ptr++ = this->hwsize;
            *ptr++ = this->prosize;

            *ptr++ = (this->opcode >> 8) & 0xFF;
            *ptr++ = this->opcode & 0xFF;

            ptr = this->smac.to_bytes(ptr);
            ptr = this->sip.to_bytes(ptr);
            ptr = this->dmac.to_bytes(ptr);
            ptr = this->dip.to_bytes(ptr);

            return ptr;
        }
        constexpr size_t size() { return 8 + this->hwsize * 2 + this->prosize * 2; }

        static constexpr frame query_for(mac::address smac, ipv4::address sip, ipv4::address dip)
        {
            return { hwtypes::ethernet, protypes::ipv4, 6, 4, opcodes::arp_request, smac, sip, { }, dip };
        }

        static constexpr frame reply_for(mac::address smac, ipv4::address sip, mac::address dmac, ipv4::address dip)
        {
            return { hwtypes::ethernet, protypes::ipv4, 6, 4, opcodes::arp_reply, smac, sip, dmac, dip };
        }
    };

    struct route
    {
        ipv4::address ip;
        mac::address mac;
        bool resolved;

        event::simple::event_t doorbell;

        constexpr bool operator==(const route &rhs) const
        {
            return this->ip == rhs.ip && this->mac == rhs.mac;
        }
    };

    struct processor
    {
        private:
        std::vector<std::unique_ptr<route>> routes {
            std::make_unique<route>(ipv4::broadcast, mac::broadcast, true)
        };
        sender *_sender;

        void submit_query(ipv4::address dip);
        void submit_reply(mac::address dmac, ipv4::address dip);

        public:
        using from_frame_type = ethernet::frame;

        processor() : routes(), _sender(nullptr) { }

        void attach_sender(sender *_sender) { this->_sender = _sender; }
        void push_packet(mem::buffer &&buffer, ethernet::frame &&frame);
        mac::address mac_of(ipv4::address ipv4);
        bool matches(const ethernet::frame &frame);
    };
    static_assert(is_processor<processor>);
} // namespace net::arp