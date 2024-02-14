// Copyright (C) 2022-2024  ilobilo

#include <drivers/fs/dev/tty/tty.hpp>
#include <drivers/fs/devtmpfs.hpp>
#include <drivers/term.hpp>
#include <init/kernel.hpp>
#include <lib/panic.hpp>
#include <lib/alloc.hpp>
#include <lib/log.hpp>
#include <unifont.h>

namespace term
{
    std::vector<terminal_t*> terms;
    terminal_t *main_term = nullptr;
    size_t term_count = 0;

    void print(const char *str, terminal_t *term)
    {
        if (term != nullptr) [[likely]]
            term_write(term->ctx, str, strlen(str));
    }

    void printc(char c, terminal_t *term)
    {
        if (term != nullptr) [[likely]]
            term_write(term->ctx, &c, 1);
    }

    extern "C"
    {
        void *term_alloc(size_t size) { return malloc(size); }
        void *term_realloc(void *oldptr, size_t size) { return realloc(oldptr, size); }
        void term_free(void *ptr, size_t size) { (void)size; free(ptr); }
        void term_freensz(void *ptr) { free(ptr); }

        void *term_memcpy(void *dest, const void *src, size_t size) { return memcpy(dest, src, size); }
        void *term_memset(void *dest, int val, size_t count) { return memset(dest, val, count); }
    } // extern "C"

    // void terminal_t::callback(term_t *_term, uint64_t type, uint64_t first, uint64_t second, uint64_t third)
    // {
    //     auto term = reinterpret_cast<terminal_t*>(_term);
    //     switch (type)
    //     {
    //         case TERM_CB_DEC:
    //             break;
    //         case TERM_CB_BELL:
    //             break;
    //         case TERM_CB_PRIVATE_ID:
    //             break;
    //         case TERM_CB_STATUS_REPORT:
    //             break;
    //         case TERM_CB_POS_REPORT:
    //             term->x = first;
    //             term->y = second;
    //             break;
    //         case TERM_CB_KBD_LEDS:
    //             break;
    //         case TERM_CB_MODE:
    //             break;
    //         case TERM_CB_LINUX:
    //             break;
    //     }
    // }

    void early_init()
    {
        log::infoln("Terminal: Initialising...");

        auto font_address = reinterpret_cast<uintptr_t>(&unifont);
        image_t *image = nullptr;

        auto font_mod = find_module("font");
        if (font_mod != nullptr)
            font_address = reinterpret_cast<uintptr_t>(font_mod->address);

        auto back_mod = find_module("background");
        if (back_mod == nullptr)
            log::errorln("Terminal background not found!");
        else
            image = image_open(back_mod->address, back_mod->size);

        font_t font
        {
            font_address,
            UNIFONT_WIDTH,
            UNIFONT_HEIGHT,
            DEFAULT_FONT_SPACING,
            DEFAULT_FONT_SCALE_X,
            DEFAULT_FONT_SCALE_Y,
        };

        style_t style
        {
            DEFAULT_ANSI_COLOURS,
            DEFAULT_ANSI_BRIGHT_COLOURS,
            DEFAULT_BACKGROUND,
            DEFAULT_FOREGROUND_BRIGHT,
            DEFAULT_BACKGROUND_BRIGHT,
            DEFAULT_FOREGROUND_BRIGHT,
            DEFAULT_MARGIN,
            DEFAULT_MARGIN_GRADIENT
        };

        background_t back
        {
            image,
            IMAGE_CENTERED,
            DEFAULT_BACKDROP
        };

        for (size_t i = 0; i < frm::frm_count; i++)
        {
            framebuffer_t frm
            {
                reinterpret_cast<uintptr_t>(frm::frms[i]->address),
                frm::frms[i]->width,
                frm::frms[i]->height,
                frm::frms[i]->pitch
            };

            if (image && (frm::frms[i]->width > image->x_size || frm::frms[i]->height > image->y_size))
                back.style = IMAGE_STRETCHED;

            auto term = terms.emplace_back(
                new terminal_t(
                    term_init(frm, font, style, back),
                    frm::frms[i]->width,
                    frm::frms[i]->height
                )
            );

            if (main_term == nullptr)
                main_term = term;
        }
        term_count = frm::frm_count;
    }

    struct tty_t : tty::tty_t
    {
        terminal_t *term;

        tty_t(terminal_t *term) : tty::tty_t(), term(term) { }

        void print(char c)
        {
            printc(c, this->term);
        }

        int ioctl(vfs::resource *res, vfs::fdhandle *fd, size_t request, uintptr_t argp)
        {
            switch (request)
            {
                case tiocgwinsz:
                    *reinterpret_cast<winsize*>(argp) = {
                        uint16_t(this->term->ctx->cols),
                        uint16_t(this->term->ctx->rows),
                        uint16_t(this->term->xpix),
                        uint16_t(this->term->ypix)
                    };
                    return 0;
                default:
                    // fmt::print("TTY: IOCTL: Unknown request 0x{:X}\n", request);
                    return 0;
            }
            std::unreachable();
        }
    };

    void init()
    {
        for (size_t i = 0; const auto &term : terms)
        {
            auto dev = makedev(4, i);
            auto tty = new tty_t(term);

            tty::register_tty(dev, tty);
            devtmpfs::add_dev("tty"s + std::to_string(i++), dev, 0620 | s_ifchr);
        }
    }
} // namespace term