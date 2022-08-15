// Copyright (C) 2022  ilobilo

#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/mm/vmm.hpp>
#include <kernel/kernel.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/pmm.hpp>

namespace mm::vmm
{
    static constexpr uint64_t psize_4kb = 0x1000;
    static constexpr uint64_t psize_2mb = 0x200000;
    static constexpr uint64_t psize_1gb = 0x40000000;

    static PTable *get_next_lvl(PTable *curr_lvl, size_t entry, bool allocate = true)
    {
        if (curr_lvl == nullptr)
            return nullptr;

        PTable *ret = nullptr;

        if (curr_lvl->entries[entry].getflags(Present))
            ret = reinterpret_cast<PTable*>(curr_lvl->entries[entry].getAddr() << 12);
        else if (allocate == true)
        {
            ret = pmm::alloc<PTable*>();
            curr_lvl->entries[entry].setAddr(reinterpret_cast<uint64_t>(ret) >> 12);
            curr_lvl->entries[entry].setflags(Present | Write | UserSuper, true);
        }
        return ret;
    }

    static PDEntry *virt2pte(PTable *toplvl, uint64_t vaddr, bool allocate, uint64_t psize)
    {
        size_t pml5_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 48)) >> 48;
        size_t pml4_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 39)) >> 39;
        size_t pml3_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 30)) >> 30;
        size_t pml2_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 21)) >> 21;
        size_t pml1_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 12)) >> 12;

        PTable *pml4, *pml3, *pml2, *pml1;

        pml4 = lvl5 ? get_next_lvl(toplvl, pml5_entry, allocate) : toplvl;
        if (pml4 == nullptr)
            return nullptr;

        pml3 = get_next_lvl(pml4, pml4_entry, allocate);
        if (pml3 == nullptr)
            return nullptr;

        if (psize == psize_1gb)
            return &pml3->entries[pml3_entry];

        pml2 = get_next_lvl(pml3, pml3_entry, allocate);
        if (pml2 == nullptr)
            return nullptr;

        if (psize == psize_2mb)
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
            case UNCACHABLE:
                break;
            case WRITE_COMBINING:
                ret |= WriteThrough;
                break;
            case WRITE_THROUGH:
                ret |= patbit;
                break;
            case WRITE_PROTECTED:
                ret |= patbit | WriteThrough;
                break;
            case WRITE_BACK:
                ret |= patbit | CacheDisable;
                break;
            default:
                break;
        }
        return ret;
    }

    uint64_t Pagemap::virt2phys(uint64_t vaddr, bool largepages)
    {
        PDEntry *pml_entry = virt2pte(reinterpret_cast<PTable*>(this->toplvl), vaddr, false, largepages ? this->large_page_size : this->page_size);
        if (pml_entry == nullptr || !pml_entry->getflags(Present)) return 0;

        return pml_entry->getAddr() << 12;
    }

    bool Pagemap::mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags, caching cache)
    {
        lockit(this->lock);

        PDEntry *pml_entry = virt2pte(reinterpret_cast<PTable*>(this->toplvl), vaddr, true, (flags & LPAGES) ? this->large_page_size : this->page_size);
        if (pml_entry == nullptr)
        {
            log::error("VMM: Could not get page map entry!");
            return false;
        }

        uint64_t realflags = Present;

        if (flags & WRITE)
            realflags |= Write;
        if (flags & USER)
            realflags |= UserSuper;
        if (!(flags & EXEC))
            realflags |= NoExec;
        if (flags & LPAGES)
            realflags |= LargerPages;

        realflags |= cache2flags(cache, flags & LPAGES);

        pml_entry->setAddr(paddr >> 12);
        pml_entry->setflags(realflags, true);
        return true;
    }

    bool Pagemap::remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags, caching cache)
    {
        uint64_t paddr = this->virt2phys(vaddr_old);
        this->unmapMem(vaddr_old);
        this->mapMem(vaddr_new, paddr, flags, cache);

        return true;
    }

    bool Pagemap::unmapMem(uint64_t vaddr, bool largepages)
    {
        lockit(this->lock);

        PDEntry *pml_entry = virt2pte(reinterpret_cast<PTable*>(this->toplvl), vaddr, false, largepages ? this->large_page_size : this->page_size);
        if (pml_entry == nullptr)
        {
            log::error("VMM: Could not get page map entry!");
            return false;
        }

        pml_entry->value = 0;
        cpu::invlpg(vaddr);
        return true;
    }

    void Pagemap::switchTo()
    {
        write_cr(3, reinterpret_cast<uint64_t>(reinterpret_cast<PTable*>(this->toplvl)));
    }

    void Pagemap::save()
    {
        this->toplvl = reinterpret_cast<PTable*>(read_cr(3));
    }

    Pagemap::Pagemap(bool user)
    {
        uint32_t a, b, c, d;
        bool gib1_pages = cpu::id(0x80000001, 0, a, b, c, d) && ((d & 1 << 26) == 1 << 26);

        this->large_page_size = gib1_pages ? psize_1gb : psize_2mb;
        this->page_size = psize_4kb;

        this->toplvl = pmm::alloc<PTable*>();

        if (user == false)
        {
            for (size_t i = 256; i < 512; i++)
                get_next_lvl(reinterpret_cast<PTable*>(this->toplvl), i, true);

            for (uint64_t i = 0; i < 0x100000000; i += this->large_page_size)
            {
                this->mapMem(i, i, RWX | LPAGES);
                this->mapMem(tohh(i), i, RWX | LPAGES);
            }

            for (size_t i = 0; i < memmap_request.response->entry_count; i++)
            {
                limine_memmap_entry *mmap = memmap_request.response->entries[i];

                uint64_t base = align_down(mmap->base, this->page_size);
                uint64_t top = align_up(mmap->base + mmap->length, this->page_size);
                if (top < 0x100000000)
                    continue;

                caching cache = default_caching;
                if (mmap->type == LIMINE_MEMMAP_FRAMEBUFFER)
                    cache = WRITE_COMBINING;

                for (uint64_t t = base; t < top; t += this->page_size)
                {
                    if (t < 0x100000000)
                        continue;
                    this->mapMem(t, t, RWX, cache);
                    this->mapMem(tohh(t), t, RWX, cache);
                }
            }
        }

        for (size_t i = 0; i < kernel_file_request.response->kernel_file->size; i += this->page_size)
        {
            uint64_t paddr = kernel_address_request.response->physical_base + i;
            uint64_t vaddr = kernel_address_request.response->virtual_base + i;
            this->mapMem(vaddr, paddr, RWX);
        }
    }

    bool is_canonical(uintptr_t addr)
    {
        if (lvl5 == true)
            return (addr <= 0x00FFFFFFFFFFFFFFULL) || (addr >= 0xFF00000000000000ULL);
        else
            return (addr <= 0x00007FFFFFFFFFFFULL) || (addr >= 0xFFFF800000000000ULL);
    }
} // namespace mm::vmm