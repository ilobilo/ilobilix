// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <lib/net/ipv4.hpp>

namespace net::ipv4::icmp
{
    enum types : uint8_t
    {
        echo_request = 8,
        echo_reply = 0
    };

    struct frame
    {
        uint8_t type;
        uint8_t code;
        uint16_t csum;

        uint16_t ident;
        uint16_t seq;

        uint8_t *payload;
        size_t payload_size;

        static constexpr frame from_ipv4(const ipv4::frame &data)
        {
            const uint8_t *bytes = data.payload;
            frame ret;
            ret.type = bytes[0];
            ret.code = bytes[1];
            ret.csum = (uint16_t(bytes[0]) << 8) | bytes[1];

            switch (ret.type)
            {
                case types::echo_request:
                case types::echo_reply:
                    ret.ident = (uint16_t(bytes[4]) << 8) | bytes[5];
                    ret.seq = (uint16_t(bytes[6]) << 8) | bytes[7];
                    ret.payload = const_cast<uint8_t *>(bytes) + 8;
                    ret.payload_size = data.payload_size - 8;
                    break;
                default:
                    break;
            }
            return ret;
        }

        constexpr uint8_t *to_bytes(uint8_t *ptr)
        {
            *ptr++ = this->type;
            *ptr++ = this->code;

            // checksum
            *ptr++ = 0;
            *ptr++ = 0;

            switch (this->type)
            {
                case types::echo_request:
                case types::echo_reply:
                    *ptr++ = this->ident >> 8;
                    *ptr++ = this->ident & 0xFF;

                    *ptr++ = this->seq >> 8;
                    *ptr++ = this->seq & 0xFF;

                    memcpy(ptr, this->payload, this->payload_size);
                    ptr += this->payload_size;
                    break;
                default:
                    break;
            }

            return ptr;
        }

        constexpr void do_checksum(uint8_t *buf, ptrdiff_t offset, size_t l)
        {
            auto hdr = buf + offset;
            uint16_t sum = checksum::compute(hdr, this->size());
            hdr[2] = sum >> 8;
            hdr[3] = sum & 0xFF;
        }

        constexpr size_t size()
        {
            switch (this->type)
            {
                case types::echo_request:
                case types::echo_reply:
                    return this->payload_size + 8;
                    break;
                default:
                    return 4;
            }
            std::unreachable();
        }
    };

    struct processor
    {
        private:
        sender *_sender;

        public:
        using from_frame_type = ipv4::frame;

        processor() : _sender(nullptr) { }

        void attach_sender(sender *s) { this->_sender = s; }

        void push_packet(mem::u8buffer &&buffer, ipv4::frame &&frame);
        bool matches(const ipv4::frame &frame);
    };
    static_assert(is_processor<processor>);
} // namespace net::ipv4::icmp