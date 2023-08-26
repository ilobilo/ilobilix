// Copyright (C) 2022-2023  ilobilo

#include <limine_terminal/term.h>

#include <drivers/frm.hpp>
#include <lib/misc.hpp>
#include <vector>
#include <mutex>

namespace term
{
    struct terminal_t
    {
        std::mutex lock;
        term_t *ctx;

        size_t xpix;
        size_t ypix;

        terminal_t(term_t *ctx, size_t xpix, size_t ypix) : ctx(ctx), xpix(xpix), ypix(ypix) { }
    };

    extern std::vector<terminal_t*> terms;
    extern terminal_t *main_term;
    extern size_t term_count;

    void print(const char *str, terminal_t *term = main_term);
    void printc(char c, terminal_t *term = main_term);

    void early_init();
    void init();
} // namespace term