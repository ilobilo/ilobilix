// Copyright (C) 2022-2023  ilobilo

#include <limine_terminal/term.h>

#include <drivers/frm.hpp>
#include <lib/misc.hpp>
#include <lib/lock.hpp>
#include <vector>

namespace term
{
    struct terminal_t
    {
        lock_t lock;
        term_context *ctx;

        size_t xpix;
        size_t ypix;

        terminal_t(term_context *ctx, size_t xpix, size_t ypix) : ctx(ctx), xpix(xpix), ypix(ypix) { }
    };

    extern std::vector<terminal_t*> terms;
    extern terminal_t *main_term;
    extern size_t term_count;

    extern limine_terminal *early_term;

    void print(const char *str, terminal_t *term = main_term);
    void printc(char c, terminal_t *term = main_term);

    void early_init();
    void init();
    void late_init();
} // namespace term