// Copyright (C) 2022  ilobilo

#include <drivers/frm.hpp>
#include <init/kernel.hpp>
#include <lib/log.hpp>

namespace frm
{
    std::vector<limine_framebuffer*> frms;
    limine_framebuffer *main_frm = nullptr;
    size_t frm_count = 0;

    void init()
    {
        log::infoln("Framebuffer: Initialising...");

        auto response = framebuffer_request.response;
        frm_count = response->framebuffer_count;

        for (size_t i = 0; i < frm_count; i++)
            frms.push_back(response->framebuffers[i]);

        main_frm = frms.front();
    }

    // TODO
    void late_init()
    {
    }
} // namespace frm