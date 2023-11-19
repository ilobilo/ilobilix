// Copyright (C) 2022-2023  ilobilo

#include <arch/x86_64/cpu/cpu.hpp>
#include <init/kernel.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

namespace vmm
{
    enum
    {
        Present = (1 << 0),
        Write = (1 << 1),
        UserSuper = (1 << 2),
        WriteThrough = (1 << 3), // PWT
        CacheDisable = (1 << 4), // PCD
        Accessed = (1 << 5),
        LargerPages = (1 << 7),
        PAT4k = (1 << 7), // PAT lvl1
        Global = (1 << 8),
        Custom0 = (1 << 9),
        Custom1 = (1 << 10),
        Custom2 = (1 << 11),
        PATlg = (1 << 12), // PAT lvl2+
        NoExec = (1UL << 63)
    };
    struct [[gnu::packed]] ptable { ptentry entries[512]; };
    static bool gib1_pages = false;

    uintptr_t pa_mask = 0x000FFFFFFFFFF000;

    static ptable *get_next_lvl(ptable *curr_lvl, size_t entry, bool allocate = true)
    {
        if (curr_lvl == nullptr)
            return nullptr;

        ptable *ret = nullptr;

        if (curr_lvl->entries[entry].getflags(Present))
            ret = reinterpret_cast<ptable*>(tohh(curr_lvl->entries[entry].getaddr()));
        else if (allocate == true)
        {
            ret = new ptable;
            curr_lvl->entries[entry].setaddr(fromhh(reinterpret_cast<uint64_t>(ret)));
            curr_lvl->entries[entry].setflags(Present | Write | UserSuper, true);
        }
        return ret;
    }

    ptentry *pagemap::virt2pte(uint64_t vaddr, bool allocate, uint64_t psize)
    {
        size_t pml5_entry = (vaddr & (0x1FFULL << 48)) >> 48;
        size_t pml4_entry = (vaddr & (0x1FFULL << 39)) >> 39;
        size_t pml3_entry = (vaddr & (0x1FFULL << 30)) >> 30;
        size_t pml2_entry = (vaddr & (0x1FFULL << 21)) >> 21;
        size_t pml1_entry = (vaddr & (0x1FFULL << 12)) >> 12;

        ptable *pml4, *pml3, *pml2, *pml1;

        pml4 = if_max_pgmode(get_next_lvl(this->toplvl, pml5_entry, allocate)) : this->toplvl;
        if (pml4 == nullptr)
            return nullptr;

        pml3 = get_next_lvl(pml4, pml4_entry, allocate);
        if (pml3 == nullptr)
            return nullptr;

        if (psize == this->llpage_size || pml3->entries[pml3_entry].getflags(LargerPages))
            return &pml3->entries[pml3_entry];

        pml2 = get_next_lvl(pml3, pml3_entry, allocate);
        if (pml2 == nullptr)
            return nullptr;

        if (psize == this->lpage_size || pml2->entries[pml2_entry].getflags(LargerPages))
            return &pml2->entries[pml2_entry];

        pml1 = get_next_lvl(pml2, pml2_entry, allocate);
        if (pml1 == nullptr)
            return nullptr;

        return &pml1->entries[pml1_entry];
    }

    /*
     * Uncachable:     PAT0:  PAT = 0, PCD = 0, PWT = 0
     * WriteCombining: PAT1:  PAT = 0, PCD = 0, PWT = 1
     * None            PAT2:  PAT = 0, PCD = 1, PWT = 0
     * None            PAT3:  PAT = 0, PCD = 1, PWT = 1
     * WriteThrough:   PAT4:  PAT = 1, PCD = 0, PWT = 0
     * WriteProtected: PAT5:  PAT = 1, PCD = 0, PWT = 1
     * WriteBack:      PAT6:  PAT = 1, PCD = 1, PWT = 0
     * Uncached:       PAT7:  PAT = 1, PCD = 1, PWT = 1
     */

    static uint64_t cache2flags(caching cache, bool largepages)
    {
        uint64_t patbit = (largepages ? PATlg : PAT4k);
        uint64_t ret = 0;
        switch (cache)
        {
            case uncachable:
                break;
            case write_combining:
                ret |= WriteThrough;
                break;
            case write_through:
                ret |= patbit;
                break;
            case write_protected:
                ret |= patbit | WriteThrough;
                break;
            case write_back:
                ret |= patbit | CacheDisable;
                break;
            default:
                break;
        }
        return ret;
    }

    uintptr_t pagemap::virt2phys(uintptr_t vaddr, size_t flags)
    {
        std::unique_lock guard(lock);

        auto psize = this->get_psize(flags);
        ptentry *pml_entry = this->virt2pte(vaddr, false, psize);
        if (pml_entry == nullptr || !pml_entry->getflags(Present))
            return invalid_addr;

        return pml_entry->getaddr() + (vaddr % psize);
    }

    bool pagemap::map(uintptr_t vaddr, uintptr_t paddr, size_t flags, caching cache)
    {
        std::unique_lock guard(this->lock);

        auto map_one = [this](uintptr_t vaddr, uintptr_t paddr, size_t flags, caching cache, size_t psize)
        {
            ptentry *pml_entry = this->virt2pte(vaddr, true, psize);
            if (pml_entry == nullptr)
            {
                if (print_errors)
                    log::errorln("VMM: Could not get page map entry for address 0x{:X}", vaddr);
                return false;
            }

            auto realflags = flags2arch(flags) | cache2flags(cache, psize != this->page_size);

            pml_entry->reset();
            pml_entry->setaddr(paddr);
            pml_entry->setflags(realflags, true);
            return true;
        };

        auto psize = this->get_psize(flags);
        if (psize == this->llpage_size && gib1_pages == false)
        {
            flags &= ~llpage;
            flags |= lpage;
            for (size_t i = 0; i < gib1; i += mib2)
                if (!map_one(vaddr + i, paddr + i, flags, cache, mib2))
                    return false;
            return true;
        }

        return map_one(vaddr, paddr, flags, cache, psize);
    }

    bool pagemap::unmap_nolock(uintptr_t vaddr, size_t flags)
    {
        auto unmap_one = [this](uintptr_t vaddr, size_t psize)
        {
            ptentry *pml_entry = this->virt2pte(vaddr, false, psize);
            if (pml_entry == nullptr)
            {
                if (print_errors)
                    log::errorln("VMM: Could not get page map entry for address 0x{:X}", vaddr);
                return false;
            }

            pml_entry->reset();
            cpu::invlpg(vaddr);
            return true;
        };

        auto psize = this->get_psize(flags);
        if (psize == this->llpage_size && gib1_pages == false)
        {
            flags &= ~llpage;
            flags |= lpage;
            for (size_t i = 0; i < gib1; i += mib2)
                if (!unmap_one(vaddr + i, mib2))
                    return false;
            return true;
        }

        return unmap_one(vaddr, psize);
    }

    bool pagemap::setflags_nolock(uintptr_t vaddr, size_t flags, caching cache)
    {
        auto psize = this->get_psize(flags);
        ptentry *pml_entry = this->virt2pte(vaddr, true, psize);
        if (pml_entry == nullptr)
        {
            if (print_errors)
                log::errorln("VMM: Could not get page map entry for address 0x{:X}", vaddr);
            return false;
        }

        auto realflags = flags2arch(flags) | cache2flags(cache, psize != this->page_size);
        auto addr = pml_entry->getaddr();

        pml_entry->reset();
        pml_entry->setaddr(addr);
        pml_entry->setflags(realflags, true);
        return true;
    }

    void pagemap::load()
    {
        wrreg(cr3, fromhh(reinterpret_cast<uint64_t>(this->toplvl)));
    }

    void pagemap::save()
    {
        this->toplvl = reinterpret_cast<ptable*>(tohh(rdreg(cr3)));
    }

    pagemap::pagemap() : toplvl(new ptable)
    {
        this->llpage_size = gib1;
        this->lpage_size = mib2;
        this->page_size = kib4;

        if (kernel_pagemap == nullptr)
        {
            for (size_t i = 256; i < 512; i++)
                get_next_lvl(this->toplvl, i, true);

            cpu::enablePAT();
            return;
        }

        for (size_t i = 256; i < 512; i++)
            this->toplvl->entries[i] = kernel_pagemap->toplvl->entries[i];
    }

    bool is_canonical(uintptr_t addr)
    {
        return if_max_pgmode((addr <= 0x00FFFFFFFFFFFFFFULL) || (addr >= 0xFF00000000000000ULL)) :
                             (addr <= 0x00007FFFFFFFFFFFULL) || (addr >= 0xFFFF800000000000ULL);
    }

    uintptr_t flags2arch(size_t flags)
    {
        uintptr_t ret = Present;
        if (flags & write)
            ret |= Write;
        if (flags & user)
            ret |= UserSuper;
        if (!(flags & exec))
            ret |= NoExec;
        if (flags & global)
            ret |= Global;
        if (islpage(flags))
            ret |= LargerPages;
        return ret;
    }

    static void destroy_level(ptable *pml, size_t start, size_t end, size_t level)
    {
        if (level == 0)
            return;

        for (size_t i = start; i < end; i++)
        {
            auto next = get_next_lvl(pml, i, false);
            if (next == nullptr)
                continue;

            destroy_level(next, 0, 512, level - 1);
        }
        delete pml;
    }

    void arch_destroy_pmap(pagemap *pmap)
    {
        destroy_level(pmap->toplvl, 0, 256, if_max_pgmode(5) : 4);
    }

    void arch_init()
    {
        uint32_t a, b, c, d;
        gib1_pages = cpu::id(0x80000001, 0, a, b, c, d) && ((d & 1 << 26) == 1 << 26);
    }
} // namespace vmm