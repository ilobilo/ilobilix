// Copyright (C) 2022  ilobilo

#include <drivers/frm/frm.hpp>
#include <main.hpp>

namespace frm
{
    vector<limine_framebuffer*> frms;
    limine_framebuffer *main_frm = nullptr;
    size_t frm_count = 0;

    void init()
    {
        auto response = framebuffer_request.response;
        frm_count = response->framebuffer_count;
        for (size_t i = 0; i < frm_count; i++)
        {
            frms.push_back(response->framebuffers[i]);
            if (main_frm == nullptr) main_frm = frms.back();
        }
    }
} // namespace frm