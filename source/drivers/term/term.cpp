// Copyright (C) 2022  ilobilo

#include <drivers/term/term.hpp>
#include <drivers/frm/frm.hpp>
#include <lib/panic.hpp>
#include <lib/log.hpp>

namespace term
{
    vector<terminal_t*> terms;
    terminal_t *main_term = nullptr;
    size_t term_count = 0;

    int printf(terminal_t *term, const char *fmt, ...)
    {
        if (term == nullptr) return -1;
        lockit(term->lock);

        auto printc = [](char c, void *arg)
        {
            auto str = reinterpret_cast<uint64_t>(&c);
            reinterpret_cast<terminal_t*>(arg)->write(str, 1);
        };

        va_list arg;
        va_start(arg, fmt);
        int ret = vfctprintf(printc, term, fmt, arg);
        va_end(arg);

        return ret;
    }

    int vprintf(terminal_t *term, const char *fmt, va_list arg)
    {
        if (term == nullptr) return -1;
        lockit(term->lock);

        auto printc = [](char c, void *arg)
        {
            auto str = reinterpret_cast<uint64_t>(&c);
            reinterpret_cast<terminal_t*>(arg)->write(str, 1);
        };

        int ret = vfctprintf(printc, term, fmt, arg);

        return ret;
    }

    extern "C" void putchar_(char c)
    {
        if (main_term == nullptr) return;
        lockit(main_term->lock);

        main_term->write(reinterpret_cast<uint64_t>(&c), 1);
    }

    extern "C" void *alloc_mem(size_t size)
    {
        return malloc(size);
    }

    extern "C" void free_mem(void *ptr, size_t size)
    {
        free(ptr);
    }

    void init()
    {
        log::info("Initialising Terminals...");

        auto callback = [](uint64_t _term, uint64_t type, uint64_t first, uint64_t second, uint64_t third)
        {
            auto term = reinterpret_cast<terminal_t*>(_term);
            switch (type)
            {
                case TERM_CB_BELL:
                    // pcspk::beep(800, 200);
                    break;
                case TERM_CB_POS_REPORT:
                    term->pos.X = first;
                    term->pos.Y = second;
                    break;
                case TERM_CB_KBD_LEDS:
                    switch (first)
                    {
                        case 0:
                            // term->tty->scrollock = false;
                            // term->tty->numlock = false;
                            // term->tty->capslock = false;
                            break;
                        case 1:
                            // term->tty->scrollock = true;
                            break;
                        case 2:
                            // term->tty->numlock = true;
                            break;
                        case 3:
                            // term->tty->capslock = true;
                            break;
                    }
                    // if (term->tty == tty::current_tty) ps2::update_leds();
                    break;
            }
        };

        if (frm::frm_count == 0)
        {
            log::error("Couldn't get a framebuffer!");
            assert(bios == true, "Booted in UEFI mode, Can't use textmode!");

            terminal_t *term = new terminal_t;

            term->init(callback, true);
            term->textmode();

            terms.push_back(term);
            main_term = term;
            return;
        }

        auto font_mod = find_module("font");
        if (font_mod == nullptr) panic("Terminal font not found!");

        auto back_mod = find_module("background");
        if (back_mod == nullptr) panic("Terminal background not found!");

        cppimage_t *image = new cppimage_t;
        image->open(reinterpret_cast<uint64_t>(back_mod->address), back_mod->size);

        font_t font
        {
            reinterpret_cast<uint64_t>(font_mod->address),
            8, 16, 1, 0, 0
        };

        style_t style
        {
            { 000000, 0xE83336, 0x31DE56, 0x624726, 0x005DFF, 0xC740F4, 0x20C7FF, 0xA3ADBF },
            DEFAULT_ANSI_BRIGHT_COLOURS,
            0xA0000000,
            0xFFFFFF,
            64, 4
        };

        background_t back
        {
            image,
            STRETCHED,
            0x00000000
        };

        for (size_t i = 0; i < frm::frm_count; i++)
        {
            terminal_t *term = new terminal_t;

            framebuffer_t frm
            {
                reinterpret_cast<uint64_t>(frm::frms[i]->address),
                frm::frms[i]->width,
                frm::frms[i]->height,
                frm::frms[i]->pitch
            };

            term->init(callback, bios);
            term->vbe(frm, font, style, back);

            terms.push_back(term);
            if (main_term == nullptr) main_term = term;
        }
        term_count = frm::frm_count;
    }
} // namespace term