// Copyright (C) 2022  ilobilo

#include <lib/misc.hpp>
#include <vector>

namespace frm
{
    extern std::vector<limine_framebuffer*> frms;
    extern limine_framebuffer *main_frm;
    extern size_t frm_count;

    void init();
} // namespace frm