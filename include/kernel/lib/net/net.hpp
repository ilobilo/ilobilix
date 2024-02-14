// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <lib/net/address.hpp>
#include <lib/buffer.hpp>
#include <cstdint>

// All of the lib/net/ code is based on:
// https://github.com/qookei/tart

namespace net
{
    struct sender
    {
        virtual ~sender() { }
        virtual void send(mem::u8buffer &&) = 0;
        virtual void receive(mem::u8buffer &&) = 0;
        virtual mac::address mac() = 0;
        virtual ipv4::address ipv4() = 0;
    };

    template<typename Type>
    concept is_processor = requires (Type a, mem::u8buffer &&b, typename Type::from_frame_type &&f, const typename Type::from_frame_type &f2, sender *s) {
        typename Type::from_frame_type;
        { a.attach_sender(s) } -> std::same_as<void>;
        { a.push_packet(std::move(b), std::move(f)) } -> std::same_as<void>;
        { a.matches(f2) } -> std::same_as<bool>;
    };

    template<typename Type>
    concept requires_checksum = requires (Type a, uint8_t *b, ptrdiff_t c, size_t d) {
        { a.do_checksum(b, c, d) } -> std::same_as<void>;
    };

    struct nic
    {
        sender *receiver = nullptr;
        void attach_receiver(sender *receiver) { this->receiver = receiver; }

        virtual net::mac::address mac_address() = 0;
        virtual bool send(mem::u8buffer &&buffer) = 0;

        virtual ~nic() { }
    };

    template<typename ...Args>
    mem::u8buffer build_packet(Args &&...args)
    {
        auto parts = std::make_tuple(std::forward<Args>(args)...);

        size_t total_size = [&parts]<size_t ...I>(std::index_sequence<I...>) {
            return (std::get<I>(parts).size() + ...);
        } (std::make_index_sequence<sizeof...(Args)> { });

        mem::u8buffer buffer(total_size);
        auto dest = static_cast<uint8_t *>(buffer.virt_data());

        [&parts, &dest]<size_t ...I>(std::index_sequence<I...>) {
            return ((dest = std::get<I>(parts).to_bytes(dest)), ...);
        } (std::make_index_sequence<sizeof...(Args)> { });

        dest = buffer.virt_data();
        [&]<size_t ...I>(std::index_sequence<I...>) {
            ([&]<typename Type>(Type item) {
                if constexpr (requires_checksum<Type>)
                {
                    ptrdiff_t off = reinterpret_cast<uintptr_t>(dest) - reinterpret_cast<uintptr_t>(buffer.virt_data());
                    item.do_checksum(buffer.virt_data(), off, total_size);
                }
                dest += item.size();
            } (std::get<I>(parts)), ...);
        } (std::make_index_sequence<sizeof...(Args)> { });

        return std::move(buffer);
    }

    template<typename From>
    void dispatch(mem::u8buffer &&, From &&, std::tuple<> &) { }

    template<typename From, is_processor ...Types>
    void dispatch(mem::u8buffer &&buf, From &&fr, std::tuple<Types...> &prs)
    {
        [&]<size_t ...I>(std::index_sequence<I...>) {
            ([&]<typename Type>(Type &proc) {
                static_assert(std::is_same_v<From, typename Type::from_frame_type>);
                if (proc.matches(fr))
                {
                    proc.push_packet(std::move(buf), std::move(fr));
                    return false;
                }
                return true;
            } (std::get<I>(prs)) && ...);
        }(std::make_index_sequence<sizeof...(Types)> { });
    }

    void register_nic(std::unique_ptr<nic> device);
} // namespace net