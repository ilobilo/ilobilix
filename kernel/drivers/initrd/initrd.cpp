// Copyright (C) 2022  ilobilo

#include <drivers/initrd/ustar.hpp>
#include <drivers/initrd/ilar.hpp>
#include <kernel/kernel.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>

namespace initrd
{
    void init()
    {
        auto mod = find_module("initrd");
        if (mod == nullptr)
        {
            log::error("Could not find initrd module!");
            return;
        }

        auto address = tohh(reinterpret_cast<uintptr_t>(mod->address));

        if (ustar::validate(address))
        {
            log::info("Initrd: Archive format is USTAR");
            ustar::init(address);
        }
        else if (ilar::validate(address))
        {
            log::info("Initrd: Archive format is ILAR");
            ilar::init(address);
        }
        else log::error("Initrd: Unknown archive format!");
    }
} // namespace initrd