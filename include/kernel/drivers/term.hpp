// Copyright (C) 2022  ilobilo

#define PRINTF_ALIAS_STANDARD_FUNCTION_NAMES 1
#include <printf/printf.h>
#include <cpp/term.hpp>
#include <lib/misc.hpp>
#include <lib/lock.hpp>
#include <vector>

namespace term
{
    struct terminal_t : cppterm_t
    {
        private:
        point pos = { 0, 0 };

        public:
        lock_t lock;

        void print(const char *str)
        {
            lockit(this->lock);
            term_print(this, str);
        }

        point getpos()
        {
            this->print("\033[6n");
            return this->pos;
        }

        static void callback(uint64_t _term, uint64_t type, uint64_t first, uint64_t second, uint64_t third);
    };

    extern std::vector<terminal_t*> terms;
    extern terminal_t *main_term;
    extern size_t term_count;

    int printf(terminal_t *term, const char *fmt, ...);
    int vprintf(terminal_t *term, const char *fmt, va_list arg);

    void init();
} // namespace term