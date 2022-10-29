// Copyright (C) 2022  ilobilo

#define PRINTF_ALIAS_STANDARD_FUNCTION_NAMES_HARD 1
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

        point getpos()
        {
            this->write("\033[6n", 5);
            return this->pos;
        }

        static void callback(term_t *_term, uint64_t type, uint64_t first, uint64_t second, uint64_t third);
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