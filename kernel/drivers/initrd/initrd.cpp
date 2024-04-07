// Copyright (C) 2022-2024  ilobilo

#include <drivers/initrd/ustar.hpp>
#include <drivers/initrd/ilar.hpp>
#include <init/kernel.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/pmm.hpp>

namespace initrd
{
    void init()
    {
        auto mod = find_module("initrd");
        if (mod == nullptr)
        {
            log::errorln("Could not find initrd module");
            return;
        }

        auto address = tohh(reinterpret_cast<uintptr_t>(mod->address));

        if (ustar::validate(address))
        {
            log::infoln("Initrd: Archive format is USTAR");
            ustar::init(address);
        }
        else if (ilar::validate(address))
        {
            log::infoln("Initrd: Archive format is ILAR");
            ilar::init(address);
        }
        else
        {
            log::errorln("Initrd: Unknown archive format");
            return;
        }

        pmm::free(fromhh(address), (align_up(mod->size, 512) + 512) / pmm::page_size);
    }
} // namespace initrd