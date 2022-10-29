// Copyright (C) 2022  ilobilo

#include <arch/aarch64/mm/vmm.hpp>
#include <init/kernel.hpp>
#include <mm/pmm.hpp>

namespace vmm
{
    uintptr_t pagemap::virt2phys(uintptr_t vaddr, bool largepages)
    {
        return 0;
    }

    bool pagemap::map(uintptr_t vaddr, uintptr_t paddr, size_t flags, caching cache)
    {
        lockit(this->lock);

        return false;
    }

    bool pagemap::remap(uintptr_t vaddr_old, uintptr_t vaddr_new, size_t flags, caching cache)
    {
        lockit(this->lock);

        return false;
    }

    bool pagemap::unmap(uintptr_t vaddr, bool largepages)
    {
        lockit(this->lock);

        return false;
    }

    void pagemap::load()
    {
    }

    void pagemap::save()
    {
    }

    pagemap::pagemap(bool user)
    {
    }

    bool is_canonical(uintptr_t addr)
    {
        return true;
    }

    void arch_init()
    {
    }
} // namespace vmm