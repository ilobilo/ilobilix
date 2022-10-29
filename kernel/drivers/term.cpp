// Copyright (C) 2022  ilobilo

#include <drivers/term.hpp>
#include <drivers/frm.hpp>
#include <lib/panic.hpp>
#include <lib/alloc.hpp>
#include <lib/log.hpp>
#include <unifont.h>

namespace term
{
    std::vector<terminal_t*> terms;
    terminal_t *main_term = nullptr;
    size_t term_count = 0;

    limine_terminal *early_term = nullptr;
    static lock_t early_lock;

    void print(const char *str, terminal_t *term)
    {
        if (term == nullptr)
        {
            if (early_term != nullptr)
            {
                lockit(early_lock);
                terminal_request.response->write(early_term, str, strlen(str));
            }
        }
        else term->write(str, strlen(str));
    }

    void printc(char c, terminal_t *term)
    {
        if (term == nullptr)
        {
            if (early_term != nullptr)
            {
                lockit(early_lock);
                terminal_request.response->write(early_term, &c, 1);
            }
        }
        else term->write(&c, 1);
    }

    extern "C"
    {
        void putchar_(char c)
        {
            printc(c);
        }

        void *alloc_mem(size_t size)
        {
            return calloc(size, 1);
        }

        void free_mem(void *ptr, size_t size)
        {
            free(ptr);
        }
    } // extern "C"

    void terminal_t::callback(term_t *_term, uint64_t type, uint64_t first, uint64_t second, uint64_t third)
    {
        auto term = reinterpret_cast<terminal_t*>(_term);
        switch (type)
        {
            case TERM_CB_DEC:
                break;
            case TERM_CB_BELL:
                break;
            case TERM_CB_PRIVATE_ID:
                break;
            case TERM_CB_STATUS_REPORT:
                break;
            case TERM_CB_POS_REPORT:
                term->pos.X = first;
                term->pos.Y = second;
                break;
            case TERM_CB_KBD_LEDS:
                break;
            case TERM_CB_MODE:
                break;
            case TERM_CB_LINUX:
                break;
        }
    }

    void early_init()
    {
        if (terminal_request.response != nullptr)
            early_term = terminal_request.response->terminals[0];
    }

    void init()
    {
        log::infoln("Initialising Terminals...");

        #if defined(__x86_64__)
        if (frm::frm_count == 0)
        {
            log::errorln("Couldn't get a framebuffer!");
            assert(uefi != true, "Booted in UEFI mode, Can't use textmode!");

            terminal_t *term = new terminal_t;

            term->init(term->callback, true);
            term->textmode();

            terms.push_back(term);
            main_term = term;

            return;
        }
        #endif

        uint64_t font_address = reinterpret_cast<uint64_t>(&unifont);
        cppimage_t *image = nullptr;

        auto font_mod = find_module("font");
        if (font_mod != nullptr)
            font_address = reinterpret_cast<uint64_t>(font_mod->address);

        auto back_mod = find_module("background");
        if (back_mod == nullptr)
            log::errorln("Terminal background not found!");
        else
        {
            image = new cppimage_t;
            image->open(reinterpret_cast<uint64_t>(back_mod->address), back_mod->size);
        }

        font_t font
        {
            font_address,
            8, 16, 1, 0, 0
        };

        style_t style
        {
            DEFAULT_ANSI_COLOURS,
            DEFAULT_ANSI_BRIGHT_COLOURS,
            0xA0000000,
            0xFFFFFF,
            static_cast<uint16_t>(image ? 64 : 0),
            static_cast<uint16_t>(image ? 4 : 0)
        };

        background_t back
        {
            image,
            CENTERED,
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

            if (frm::frms[i]->width > image->x_size || frm::frms[i]->height > image->y_size)
                back.style = STRETCHED;

            term->init(term->callback, uefi == false, 8);
            term->vbe(frm, font, style, back);

            terms.push_back(term);
            if (main_term == nullptr)
                main_term = term;
        }
        term_count = frm::frm_count;
    }

    // TODO
    void late_init()
    {
    }
} // namespace term