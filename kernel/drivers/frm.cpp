// Copyright (C) 2022-2023  ilobilo

#include <drivers/fs/devtmpfs.hpp>
#include <drivers/frm.hpp>
#include <init/kernel.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/pmm.hpp>

namespace frm
{
    std::vector<limine_framebuffer*> frms;
    limine_framebuffer *main_frm = nullptr;
    size_t frm_count = 0;

    void early_init()
    {
        log::infoln("Framebuffer: Initialising...");

        auto response = framebuffer_request.response;
        frm_count = response->framebuffer_count;

        for (size_t i = 0; i < frm_count; i++)
            frms.push_back(response->framebuffers[i]);

        main_frm = frms.front();
    }

    struct cdev_t : vfs::cdev_t
    {
        fb_fix_screeninfo fix;
        fb_var_screeninfo var;

        cdev_t(limine_framebuffer *frm)
        {
            this->fix = {
                .id = { 0 },
                .smem_start = reinterpret_cast<uintptr_t>(frm->address),
                .smem_len = static_cast<uint32_t>(frm->pitch * frm->height),
                .type = fb_type_packed_pixels,
                .visual = fb_visual_truecolor,
                .line_length = static_cast<uint32_t>(frm->pitch),
                .accel = fb_accel_none,
                .capabilities = 0,
                .reserved = { 0 }
            };
            this->var = {
                .xres = static_cast<uint32_t>(frm->width),
                .yres = static_cast<uint32_t>(frm->height),
                .xres_virtual = static_cast<uint32_t>(frm->width),
                .yres_virtual = static_cast<uint32_t>(frm->height),
                .xoffset = 0,
                .yoffset = 0,
                .bits_per_pixel = static_cast<uint32_t>(frm->bpp),
                .grayscale = 0,
                .red = {
                    .offset = frm->red_mask_shift,
                    .length = frm->red_mask_size,
                    .msb_right = 0
                },
                .green = {
                    .offset = frm->green_mask_shift,
                    .length = frm->green_mask_size,
                    .msb_right = 0
                },
                .blue = {
                    .offset = frm->blue_mask_shift,
                    .length = frm->blue_mask_size,
                    .msb_right = 0
                },
                .transp = { 0, 0, 0 },
                .nonstd = 0,
                .activate = 0,
                .height = static_cast<uint32_t>(-1),
                .width = static_cast<uint32_t>(-1),
                .accel_flags = 0,
                .pixclock = 0,
                .left_margin = 0,
                .right_margin = 0,
                .upper_margin = 0,
                .lower_margin = 0,
                .hsync_len = 0,
                .vsync_len = 0,
                .sync = 0,
                .vmode = 0,
                .rotate = 0,
                .colorspace = 0,
                .reserved = { 0 }
            };
        }

        ssize_t read(vfs::resource *res, vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
        {
            if (count == 0)
                return 0;

            if (offset + count > this->fix.smem_len)
                count = this->fix.smem_len - offset;

            memcpy(buffer, reinterpret_cast<void*>(this->fix.smem_start + offset), count);
            return count;
        }

        ssize_t write(vfs::resource *res, vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
        {
            if (count == 0)
                return 0;

            if (offset + count > this->fix.smem_len)
                count = this->fix.smem_len - offset;

            memcpy(reinterpret_cast<void*>(this->fix.smem_start + offset), buffer, count);
            return count;
        }

        void *mmap(vfs::resource *_res, size_t fpage, int flags)
        {
            size_t offset = fpage * pmm::page_size;
            if (offset >= this->fix.smem_len)
                return nullptr;

            return reinterpret_cast<void*>(fromhh(this->fix.smem_start + offset));
        }

        int ioctl(vfs::resource *res, vfs::fdhandle *fd, size_t request, uintptr_t argp)
        {
            switch (request)
            {
                case fbioget_vscreeninfo:
                    *reinterpret_cast<fb_var_screeninfo*>(argp) = this->var;
                    break;
                case fbioput_vscreeninfo:
                    this->var = *reinterpret_cast<fb_var_screeninfo*>(argp);
                    break;
                case fbioget_fscreeninfo:
                    *reinterpret_cast<fb_fix_screeninfo*>(argp) = this->fix;
                    break;
                case fbioblank:
                    break;
                default:
                    return_err(-1, EINVAL);
            }
            return 0;
        }
    };

    void init()
    {
        for (size_t i = 0; const auto &frm : frms)
        {
            auto dev = makedev(29, i);
            devtmpfs::register_dev(new cdev_t(frm), dev);
            devtmpfs::add_dev("fb"s + std::to_string(i++), dev, 0660 | s_ifchr);
        }
    }
} // namespace frm