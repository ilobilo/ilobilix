// Copyright (C) 2022  ilobilo

#include <arch/arch.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/vmm.hpp>
#include <lai/host.h>

namespace vmm
{
    pagemap *kernel_pagemap = nullptr;

    void init()
    {
        log::info("Initialising VMM...");

        if (vmm::arch_init)
            vmm::arch_init();

        kernel_pagemap = new pagemap();
        kernel_pagemap->load();
    }
} // namespace vmm

void *laihost_map(size_t address, size_t count)
{
    // vmm::kernel_pagemap->mapMemRange(tohh(address), address, count * vmm::kernel_pagemap->page_size, vmm::RW);
    return reinterpret_cast<void*>(tohh(address));
}

void laihost_unmap(void *address, size_t count)
{
    // vmm::kernel_pagemap->unmapMemRange(reinterpret_cast<uint64_t>(tohh(address)), count * vmm::kernel_pagemap->page_size);
}