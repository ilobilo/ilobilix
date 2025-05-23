// Copyright (C) 2024-2025  ilobilo

module;

#include <flanterm.h>

export module drivers.terminal;

import drivers.framebuffer;
import lib;
import cppstd;

export namespace term
{
    void write(flanterm_context *ctx, std::string_view str);
    void write(flanterm_context *ctx, char chr);

    flanterm_context *main();

    void early_init();
    void init();
} // export namespace term