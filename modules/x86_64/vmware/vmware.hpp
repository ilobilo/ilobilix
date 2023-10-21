// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <string_view>
#include <optional>
#include <string>

#include <cstdint>
#include <cstddef>

namespace vmware
{
    inline constexpr uint32_t magic = 0x564D5868;
    inline constexpr uint16_t port = 0x5658;
    inline constexpr uint16_t porthb = 0x5659;

    enum commands : uint16_t
    {
        getversion = 10,
        message = 30,
        abspointer_data = 39,
        abspointer_status = 40,
        abspointer_cmd = 41
    };

    enum abspointer : uint32_t
    {
        enable = 0x45414552,
        relative = 0xF5,
        absolute = 0x53424152
    };

    enum buttons : uint16_t
    {
        left = 0x20,
        right = 0x10,
        middle = 0x08
    };

    enum messages : uint32_t
    {
        open = 0x00000000,
        send = 0x00010000,
        receive = 0x00030000,
        ack = 0x00050000,
        close = 0x00060000
    };

    enum protocols : uint32_t
    {
        rpci = 0x49435052,
        tclo = 0x4F4C4354
    };

    struct command
    {
        union [[gnu::packed]] {
            uint32_t ax;
            uint32_t magic;
        };
        union [[gnu::packed]] {
            uint32_t bx;
            size_t size;
        };
        union [[gnu::packed]] {
            uint32_t cx;
            uint16_t cmd;
        };
        union [[gnu::packed]] {
            uint32_t dx;
            uint16_t port;
        };
        uintptr_t si;
        uintptr_t di;

        void send();
        void send_hb();
        void receive_hb();
    };

    struct channel
    {
        private:
        uint32_t protocol;
        uint32_t id;
        bool opened;

        public:
        channel(protocols proto) : protocol(proto), id(0), opened(false) { }

        bool open();
        void close();

        inline bool is_open()
        {
            return this->opened;
        }

        bool send_msg(std::string_view msg, bool addone = false);
        std::optional<std::string> receive_msg();

        static bool send_rpci(std::string_view msg);
        static channel &open_tclo();
        // static bool tclo_query();
    };
} // namespace vmware