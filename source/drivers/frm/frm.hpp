// Copyright (C) 2022  ilobilo

#include <lib/vector.hpp>
#include <lib/misc.hpp>

namespace frm
{
    extern vector<limine_framebuffer*> frms;
    extern limine_framebuffer *main_frm;
    extern size_t frm_count;

    void init();
} // namespace frm