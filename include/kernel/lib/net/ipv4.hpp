// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <lib/net/checksum.hpp>
#include <lib/net/ethernet.hpp>

namespace net::ipv4
{
    enum class protocols : uint8_t
    {
        icmp = 0x01,
        tcp = 0x06,
        udp = 0x11
    };

    struct frame
    {
        ipv4::address sip;
        ipv4::address dip;
        uint8_t proto;
        uint8_t ttl;
        uint16_t length;
        uint8_t ihl;

        uint8_t *payload;
        size_t payload_size;

        ethernet::frame ether;

        static constexpr frame from_ethernet(const ethernet::frame &data)
        {
            const uint8_t *bytes = data.payload;
            frame ret;
            ret.ether = data;
            ret.sip.from_bytes(bytes + 12);
            ret.dip.from_bytes(bytes + 16);
            ret.ihl = bytes[0] & 0xF;
            ret.ttl = bytes[8];
            ret.proto = bytes[9];
            ret.length = (uint16_t(bytes[2]) << 8) | bytes[3];
            ret.payload = const_cast<uint8_t *>(bytes) + ret.ihl * 4;
            ret.payload_size = ret.length - ret.ihl * 4;
            return ret;
        }

        constexpr uint8_t *to_bytes(uint8_t *ptr)
        {
            // TODO: options
            assert(this->ihl == 5);
            *ptr++ = (4 << 4) | this->ihl;
            *ptr++ = 0;

            uint16_t size = this->payload_size + this->ihl * 4;
            *ptr++ = size >> 8;
            *ptr++ = size & 0xFF;

            // ID
            *ptr++ = 0;
            *ptr++ = 0;

            // flags : 3, frag_offset : 13
            *ptr++ = 0;
            *ptr++ = 0;

            *ptr++ = this->ttl;
            *ptr++ = this->proto;

            // checksum
            *ptr++ = 0;
            *ptr++ = 0;

            ptr = this->sip.to_bytes(ptr);
            ptr = this->dip.to_bytes(ptr);

            return ptr;
        }

        constexpr void do_checksum(uint8_t *buf, ptrdiff_t offset, size_t l)
        {
            auto hdr = buf + offset;

            uint16_t len = l - offset;
            hdr[2] = len >> 8;
            hdr[3] = len & 0xFF;

            uint16_t sum = checksum::compute(hdr, this->size());

            hdr[10] = sum >> 8;
            hdr[11] = sum & 0xFF;
        }

        constexpr size_t size() { return this->ihl * 4; }
    };

    template<typename ...Types>
        requires (std::same_as<typename Types::from_frame_type, frame> && ...)
    struct processor
    {
        private:
        std::tuple<Types...> processors;

        public:
        using from_frame_type = ethernet::frame;

        void attach_sender(sender *) requires (sizeof...(Types) == 0) { }
        void attach_sender(sender *s) requires (sizeof...(Types) > 0)
        {
            [&]<size_t ...I>(std::index_sequence<I...>) {
                (std::get<I>(this->processors).attach_sender(s), ...);
            } (std::make_index_sequence<sizeof...(Types)> { });
        }

        void push_packet(mem::u8buffer &&buffer, ethernet::frame &&frm)
        {
            auto ipv4 = frame::from_ethernet(frm);
            dispatch(std::move(buffer), std::move(ipv4), this->processors);
        }

        bool matches(const ethernet::frame &frm)
        {
            return frm.type == ethernet::types::ipv4;
        }

        template<size_t I>
        auto &nth_processor()
        {
            return std::get<I>(this->processors);
        }
    };
    static_assert(is_processor<processor<>>);
} // namespace net::ipv4