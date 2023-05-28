// Copyright (C) 2022-2023  ilobilo

#include <arch/x86_64/cpu/idt.hpp>
#include <drivers/ps2/ps2.hpp>
#include <arch/arch.hpp>
#include <lib/panic.hpp>
#include <lib/log.hpp>

#include <drivers/fs/dev/tty/tty.hpp>
#include <drivers/ps2/scancodes.hpp>
#include <drivers/term.hpp>
#include <drivers/proc.hpp>

namespace ps2::kbd
{
    uint8_t vector = 0;

    // TODO: don't use same variables for every tty
    static union
    {
        struct
        {
            bool shift : 1;
            uint8_t rsv1 : 1;
            bool ctrl : 1;
            bool alt : 1;
            uint8_t rsv2 : 4;
        };
        uint8_t shiftstate;
    };

    static union
    {
        struct
        {
            bool capslock : 1;
            bool numlock : 1;
            bool scrollock : 1;
            uint8_t rsv0: 5;
        };
        uint8_t leds;
    };
    bool decckm = false;

    void add(char ch)
    {
        tty::active_tty->add_to_buf(ch);
    }

    void add(std::string_view str)
    {
        tty::active_tty->add_to_buf(str);
    }

    void add(std::initializer_list<char> ilist)
    {
        tty::active_tty->add_to_buf(ilist);
    }

    extern void (*handlers[])(bool, uint8_t);
    static void latin_handler(bool released, uint8_t value)
    {
        if (released == true)
            return;
        add(static_cast<char>(value));
    }
    static void func_handler(bool released, uint8_t value)
    {
        if (released == true)
            return;
        add(func_table[value]);
    }
    static void spec_handler(bool released, uint8_t value)
    {
        if (released == true)
            return;

        switch (value)
        {
            case 1:
                add('\n');
                break;
            case 7:
                capslock = !capslock;
                break;
            case 8:
                numlock = !numlock;
                break;
            case 9:
                scrollock = !scrollock;
                break;
            case 12:
                arch::reboot();
                break;
            case 13:
                capslock = true;
                break;
            case 19:
                numlock = !numlock;
                break;
        }
    }
    static void pad_handler(bool released, uint8_t value)
    {
        static constexpr auto pad_chars = "0123456789+-*/\015,.?()#";

        if (released == true)
            return;

        if (numlock == false)
        {
            switch (value)
            {
                case KEY_VALUE(KEY_PCOMMA):
                case KEY_VALUE(KEY_PDOT):
                    handlers[1](released, KEY_VALUE(KEY_REMOVE));
                    return;
                case KEY_VALUE(KEY_P0):
                    handlers[1](released, KEY_VALUE(KEY_INSERT));
                    return;
                case KEY_VALUE(KEY_P1):
                    handlers[1](released, KEY_VALUE(KEY_SELECT));
                    return;
                case KEY_VALUE(KEY_P2):
                    handlers[6](released, KEY_VALUE(KEY_DOWN));
                    return;
                case KEY_VALUE(KEY_P3):
                    handlers[1](released, KEY_VALUE(KEY_PGDN));
                    return;
                case KEY_VALUE(KEY_P4):
                    handlers[6](released, KEY_VALUE(KEY_LEFT));
                    return;
                case KEY_VALUE(KEY_P5):
                    add("\033[G");
                    return;
                case KEY_VALUE(KEY_P6):
                    handlers[6](released, KEY_VALUE(KEY_RIGHT));
                    return;
                case KEY_VALUE(KEY_P7):
                    handlers[1](released, KEY_VALUE(KEY_FIND));
                    return;
                case KEY_VALUE(KEY_P8):
                    handlers[6](released, KEY_VALUE(KEY_UP));
                    return;
                case KEY_VALUE(KEY_P9):
                    handlers[1](released, KEY_VALUE(KEY_PGUP));
                    return;
            }
        }
        add(pad_chars[value]);
    }
    static void cur_handler(bool released, uint8_t value)
    {
        if (released == true)
            return;
        static constexpr auto cur_chars = "BDCA";
        add({ '\033', (decckm ? 'O' : '['), cur_chars[value] });
    }
    static void shift_handler(bool released, uint8_t value)
    {
        if (released == true)
            shiftstate &= ~(1 << value);
        else
            shiftstate |= (1 << value);
    }
    static void meta_handler(bool released, uint8_t value)
    {
        if (released == true)
            return;
        add({ '\033', char(value) });
    }

    static void shoulnt_be_called(bool released, uint8_t value)
    {
        PANIC("PS2: bruh: {}, {}", released, value);
    }

    void (*handlers[15])(bool, uint8_t)
    {
        latin_handler, func_handler, spec_handler,
        pad_handler, shoulnt_be_called, shoulnt_be_called,
        cur_handler, shift_handler, meta_handler,
        shoulnt_be_called, shoulnt_be_called, shoulnt_be_called,
        shoulnt_be_called, shoulnt_be_called, shoulnt_be_called
    };

    event_t ev;
    void kbd_worker()
    {
        while (true)
        {
            auto scancode = read();
            if (scancode == 0xE0)
                continue;

            bool released = scancode & 0x80;
            scancode &= ~0x80;

            size_t map_i = shiftstate;
            auto map = key_maps[map_i];
            if (map == nullptr)
                map = plain_map;

            auto keysym = map[scancode];
            uint8_t type = KEY_TYPE(keysym);

            if (type >= 0xF0)
            {
                type -= 0xF0;
                if (type == KEY_TYPE_LETTER)
                {
                    type = KEY_TYPE_LATIN;
                    if (capslock == true)
                    {
                        map = key_maps[map_i ^ (1 << KG_SHIFT)];
                        if (map != nullptr)
                            keysym = map[scancode];
                    }
                }
                handlers[type](released, KEY_VALUE(keysym));
            }
            else PANIC("PS2: Type < 0xF0? 0x{:X}", type);
        }
    }

    void init()
    {
        auto [handler, _vector] = idt::allocate_handler(idt::IRQ(1));
        handler.set([](cpu::registers_t *regs) { ev.trigger(); });

        proc::enqueue(new proc::thread(kernel_proc, kbd_worker, 0));
        idt::unmask((vector = _vector) - 0x20);
    }
} // namespace ps2::kbd