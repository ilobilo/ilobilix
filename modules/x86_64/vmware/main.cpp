// Copyright (C) 2022-2024  ilobilo

#include <drivers/proc.hpp>
#include <init/kernel.hpp>
#include <lib/log.hpp>
#include <module.hpp>

#include "vmware.hpp"

namespace vmware
{
    void command::send()
    {
        this->magic = vmware::magic;
        this->port = vmware::port;
        asm volatile("in %%dx, %0" : "+a"(this->ax), "+b"(this->bx), "+c"(this->cx), "+d"(this->dx), "+S"(this->si), "+D"(this->di) :: "memory");
    }

    void command::send_hb()
    {
        this->magic = vmware::magic;
        this->port = vmware::porthb;
        asm volatile("cld; rep; outsb" : "+a"(this->ax), "+b"(this->bx), "+c"(this->cx), "+d"(this->dx), "+S"(this->si), "+D"(this->di));
    }

    void command::receive_hb()
    {
        this->magic = vmware::magic;
        this->port = vmware::porthb;
        asm volatile("cld; rep; insb" : "+a"(this->ax), "+b"(this->bx), "+c"(this->cx), "+d"(this->dx), "+S"(this->si), "+D"(this->di));
    }

    bool channel::open()
    {
        if (opened == true)
            return true;

        command cmd;
        {
            cmd.cx = static_cast<uint32_t>(commands::message) | messages::open;
            cmd.bx = this->protocol;
            cmd.send();
        }

        if ((cmd.ax & 0x10000) == 0)
            return opened = false;

        this->id = cmd.dx >> 16;
        return opened = true;
    }

    void channel::close()
    {
        if (this->opened == false)
            return;

        command cmd;
        {
            cmd.cx = static_cast<uint32_t>(commands::message) | messages::close;
            cmd.bx = 0;
            cmd.dx = this->id << 16;
            cmd.send();
        };
    }

    bool channel::send_msg(std::string_view msg, bool addone)
    {
        {
            command cmd;
            {
                cmd.cx = static_cast<uint32_t>(commands::message) | messages::send;
                cmd.size = msg.length() + size_t(addone);
                cmd.dx = this->id << 16;
                cmd.send();
            }

            if (msg.empty())
                return true;

            if (((cmd.cx >> 16) & 0x81) != 0x81)
                return false;
        }
        {
            command cmd;
            {
                cmd.bx = 0x0010000;
                cmd.cx = msg.length() + size_t(addone);
                cmd.dx = this->id << 16;
                cmd.si = reinterpret_cast<uintptr_t>(msg.data());
                cmd.send_hb();

                if (!(cmd.bx & 0x0010000))
                    return false;
            }
        }
        return true;
    }

    std::optional<std::string> channel::receive_msg()
    {
        size_t size;
        {
            command cmd;
            {
                cmd.cx = static_cast<uint32_t>(commands::message) | messages::receive;
                cmd.dx = this->id << 16;
                cmd.send();
            }

            size = cmd.bx;
            if (size == 0)
                return "";

            if (((cmd.cx >> 16) & 0x83) != 0x83)
                return std::nullopt;
        }

        std::string ret;
        ret.reserve(size);
        {
            command cmd;
            {
                cmd.bx = 0x00010000;
                cmd.cx = size;
                cmd.dx = this->id << 16;
                cmd.di = reinterpret_cast<uintptr_t>(ret.data());
                cmd.receive_hb();
            }

            if (!(cmd.bx & 0x00010000))
                return std::nullopt;
        }

        command cmd;
        {
            cmd.cx = static_cast<uint32_t>(commands::message) | messages::ack;
            cmd.bx = 1;
            cmd.dx = this->id << 16;
            cmd.send();
        }

        ret.at(ret.length()) = 0;
        return std::move(ret);
    }

    bool channel::send_rpci(std::string_view msg)
    {
        channel chn(protocols::rpci);
        if (chn.open() == false)
            return false;

        chn.send_msg(msg, true);
        auto str = chn.receive_msg();

        chn.close();
        return str.has_value();
    }

    static channel tclo_chn { protocols::tclo };
    channel &channel::open_tclo()
    {
        if (tclo_chn.is_open())
            tclo_chn.close();

        tclo_chn.open();
        return tclo_chn;
    }

    static bool detect()
    {
        command cmd;
        {
            cmd.bx = ~magic;
            cmd.cmd = commands::getversion;
            cmd.send();
        }
        if (cmd.bx != magic || cmd.ax == 0xFFFFFFFF)
            return false;
        return true;
    }

    static void toggle_mouse(bool absolute)
    {
        command cmd;
        if (absolute == true)
        {
            cmd.bx = abspointer::enable;
            cmd.cmd = commands::abspointer_cmd;
            cmd.send();

            cmd.bx = 0;
            cmd.cmd = commands::abspointer_status;
            cmd.send();

            cmd.bx = 1;
            cmd.cmd = commands::abspointer_data;
            cmd.send();

            cmd.bx = abspointer::absolute;
            cmd.cmd = commands::abspointer_cmd;
            cmd.send();
        }
        else
        {
            cmd.bx = abspointer::relative;
            cmd.cmd = commands::abspointer_cmd;
            cmd.send();
        }
    }

    struct mouse
    {
        bool left, right, middle;
        uint16_t x;
        uint16_t y;
        int8_t z;

        std::pair<size_t, size_t> map2fb(size_t width, size_t height)
        {
            return { (this->x * width) / 0xFFFF, (this->y * height) / 0xFFFF };
        }
    };

    static std::optional<mouse> handle_mouse()
    {
        command cmd;
        {
            cmd.bx = 0;
            cmd.cmd = commands::abspointer_status;
            cmd.send();
        }

        if (cmd.ax == 0xFFFF0000)
        {
            toggle_mouse(false);
            toggle_mouse(true);
            return std::nullopt;
        }

        if (auto words = (cmd.ax & 0xFFFF); (words == 0) || (words % 4))
            return std::nullopt;

        {
            cmd.bx = 4;
            cmd.cmd = commands::abspointer_data;
            cmd.send();
        }

        [[maybe_unused]]
        auto flags = (cmd.ax & 0xFFFF0000) >> 16;

        uint16_t buttons = cmd.ax & 0xFFFF;
        uint16_t x = cmd.bx;
        uint16_t y = cmd.cx;
        int8_t z = cmd.dx;

        return mouse {
            .left = (buttons & 0x20) != 0,
            .right = (buttons & 0x10) != 0,
            .middle = (buttons & 0x08) != 0,
            .x = x, .y = y, .z = z
        };
    }
} // namespace vmware

static void runner()
{
    vmware::toggle_mouse(true);
    while (true)
    {
        auto ms = vmware::handle_mouse();
        if (ms.has_value() == false)
            continue;

        // TODO: Handle Mouse Input
    }
}

DRIVER(vmware_tools, init, fini)

__init__ bool init()
{
    if (vmware::detect() == false)
        return false;

    proc::enqueue(new proc::thread(kernel_proc, runner, 0));
    return true;
}

__fini__ bool fini()
{
    assert(false, "VMWare->fini() not implemented!");
    return false;
}