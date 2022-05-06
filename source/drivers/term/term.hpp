// Copyright (C) 2022  ilobilo

#define PRINTF_ALIAS_STANDARD_FUNCTION_NAMES 1
#include <printf/printf.h>
#include <lib/vector.hpp>
#include <cpp/term.hpp>
#include <lib/misc.hpp>

namespace term
{
    struct terminal_t : cppterm_t
    {
        lock_t lock;
        point pos = { 0, 0 };

        void print(const char *str)
        {
            lockit(this->lock);
            term_print(this, str);
        }
    };

    extern vector<terminal_t*> terms;
    extern terminal_t *main_term;
    extern size_t term_count;

    int printf(terminal_t *term, const char *fmt, ...);

    void init();
} // namespace term