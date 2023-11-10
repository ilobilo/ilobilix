// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <lib/net/net.hpp>
#include <lib/event.hpp>
#include <deque>

namespace net::ethernet
{
    enum types : uint16_t
    {
        ipv4 = 0x0800,
        ipv6 = 0x86DD,
        arp = 0x0806,
    };

    struct frame
    {
        mac::address source;
        mac::address dest;
        uint16_t type;
        uint8_t *payload;
        size_t payload_size;

        static constexpr bool is_valid(uint8_t *data)
        {
            auto type_or_length = (uint16_t(data[12]) << 8) | data[13];
            if (type_or_length <= 0x05DC)
                return false; // IEEE 802.3
            return true;
        }

        static constexpr frame from_bytes(uint8_t *data, size_t size)
        {
            frame ret;
            ret.dest.from_bytes(data);
            ret.source.from_bytes(data + 6);
            ret.type = (uint16_t(data[12]) << 8) | data[13];
            ret.payload = data + 14;
            ret.payload_size = size - 18;
            return ret;
        }

        constexpr uint8_t *to_bytes(uint8_t *ptr)
        {
            ptr = this->dest.to_bytes(ptr);
            ptr = this->source.to_bytes(ptr);
            *ptr++ = (this->type >> 8) & 0xFF;
            *ptr++ = this->type & 0xFF;
            return ptr;
        }
        constexpr size_t size() { return 14; }
    };

    template<is_processor ...Types>
        requires (std::same_as<typename Types::from_frame_type, ethernet::frame> && ...)
    struct dispatcher : sender
    {
        private:
        std::tuple<Types...> processors;
        ipv4::address ip;
        nic *_nic;

        std::deque<mem::u8buffer> packets;
        event::simple::event_t event;

        void attach_senders() requires (sizeof...(Types) == 0) { }
        void attach_senders() requires (sizeof...(Types) > 0)
        {
            [&]<size_t ...I>(std::index_sequence<I...>) {
                (std::get<I>(this->processors).attach_sender(this), ...);
            } (std::make_index_sequence<sizeof...(Types)> { });
        }

        public:
        friend void swap(dispatcher &lhs, dispatcher &rhs)
        {
            using std::swap;
            swap(lhs.processors, rhs.processors);
            swap(lhs.ip, rhs.ip);
            swap(lhs._nic, rhs._nic);
            swap(lhs.packets, rhs.packets);
            // swap(lhs.event, rhs.event); // not swapping the event shouldn't be a problem
        }

        dispatcher(nic *_nic) : processors { }, _nic(_nic) { this->attach_senders(); }
        dispatcher(dispatcher &&other) noexcept { swap(*this, other); }

        static void runner(dispatcher *ptr)
        {
            while (true)
            {
                if (ptr->packets.empty())
                    ptr->event.await();

                while (ptr->packets.empty() == false)
                {
                    auto packet = std::move(ptr->packets.pop_back_element());
                    if (frame::is_valid(packet.virt_data()) == false)
                        continue;

                    auto ether = frame::from_bytes(packet.virt_data(), packet.size());
                    dispatch(std::move(packet), std::move(ether), ptr->processors);
                }
            }
        }

        void send(mem::u8buffer &&buffer) override
        {
            this->_nic->send(std::move(buffer));
        }

        void receive(mem::u8buffer &&buffer) override
        {
            this->packets.emplace_front(std::move(buffer));
            this->event.trigger();
        }

        mac::address mac() override
        {
            return this->_nic->mac_address();
        }

        ipv4::address ipv4() override
        {
            return this->ip;
        }

        void set_ipv4(ipv4::address ip)
        {
            this->ip = ip;
        }

        template<size_t I>
        auto &nth_processor()
        {
            return std::get<I>(this->processors);
        }
    };
} // namespace net::ethernet