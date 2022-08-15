// Copyright (C) 2022  ilobilo

#include <arch/arch.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/vmm.hpp>
#include <lai/host.h>

namespace mm::vmm
{
    Pagemap *kernel_pagemap = nullptr;

    void init()
    {
        log::info("Initialising VMM...");

        if (arch::vmm_init)
            arch::vmm_init();

        kernel_pagemap = new Pagemap();
        kernel_pagemap->switchTo();
    }
} // namespace mm::vmm

void *laihost_map(size_t address, size_t count)
{
    // mm::vmm::kernel_pagemap->mapMemRange(tohh(address), address, count * mm::vmm::kernel_pagemap->page_size, mm::vmm::RW);
    return reinterpret_cast<void*>(tohh(address));
}

void laihost_unmap(void *address, size_t count)
{
    // mm::vmm::kernel_pagemap->unmapMemRange(reinterpret_cast<uint64_t>(tohh(address)), count * mm::vmm::kernel_pagemap->page_size);
}