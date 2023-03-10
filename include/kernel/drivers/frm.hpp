// Copyright (C) 2022-2023  ilobilo

#include <lib/types.hpp>
#include <limine.h>
#include <vector>

namespace frm
{
    extern std::vector<limine_framebuffer*> frms;
    extern limine_framebuffer *main_frm;
    extern size_t frm_count;

    void init();
    void late_init();
} // namespace frm