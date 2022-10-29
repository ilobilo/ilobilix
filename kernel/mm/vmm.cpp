// Copyright (C) 2022  ilobilo

#include <arch/arch.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <lai/host.h>
#include <mm/vmm.hpp>

namespace vmm
{
    pagemap *kernel_pagemap = nullptr;

    void init()
    {
        log::infoln("Initialising VMM...");

        if (vmm::arch_init)
            vmm::arch_init();

        kernel_pagemap = new pagemap();
        kernel_pagemap->load();
    }
} // namespace vmm

void *laihost_map(size_t address, size_t count)
{
    return reinterpret_cast<void*>(tohh(address));
}

void laihost_unmap(void *address, size_t count) { }