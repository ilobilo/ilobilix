// Copyright (C) 2022  ilobilo

#include <mm/pmm/pmm.hpp>
#include <mm/vmm/vmm.hpp>
#include <lib/lock.hpp>
#include <lib/misc.hpp>
#include <lib/cpu.hpp>
#include <lib/log.hpp>
#include <algorithm>
#include <main.hpp>

namespace mm::vmm
{
    Pagemap *kernel_pagemap = nullptr;
    bool lvl5 = LVL5_PAGING;

    PTable *get_next_lvl(PTable *curr_lvl, size_t entry, bool allocate = true)
    {
        PTable *ret = nullptr;
        if (curr_lvl->entries[entry].getflag(Present))
        {
            ret = reinterpret_cast<PTable*>(static_cast<uint64_t>(curr_lvl->entries[entry].getAddr()) << 12);
        }
        else if (allocate == true)
        {
            ret = pmm::alloc<PTable*>();
            curr_lvl->entries[entry].setAddr(reinterpret_cast<uint64_t>(ret) >> 12);
            curr_lvl->entries[entry].setflags(Present | ReadWrite | UserSuper, true);
        }
        return ret;
    }

    PDEntry *Pagemap::virt2pte(uint64_t vaddr, bool allocate, bool hugepages)
    {
        size_t pml5_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 48)) >> 48;
        size_t pml4_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 39)) >> 39;
        size_t pml3_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 30)) >> 30;
        size_t pml2_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 21)) >> 21;
        size_t pml1_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 12)) >> 12;

        PTable *pml5, *pml4, *pml3, *pml2, *pml1;

        if (lvl5)
        {
            pml5 = this->TOPLVL;
            if (pml5 == nullptr) return nullptr;

            pml4 = get_next_lvl(pml5, pml5_entry, allocate);
        }
        else
        {
            pml5 = nullptr;
            pml4 = this->TOPLVL;
        }
        if (pml4 == nullptr) return nullptr;

        pml3 = get_next_lvl(pml4, pml4_entry, allocate);
        if (pml3 == nullptr) return nullptr;

        pml2 = get_next_lvl(pml3, pml3_entry, allocate);
        if (pml2 == nullptr) return nullptr;
        if (hugepages) return &pml2->entries[pml2_entry];

        pml1 = get_next_lvl(pml2, pml2_entry, allocate);
        if (pml1 == nullptr) return nullptr;

        return &pml1->entries[pml1_entry];
    }

    void Pagemap::mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags, bool hugepages)
    {
        lockit(this->lock);

        PDEntry *pml_entry = this->virt2pte(vaddr, true, hugepages);
        if (pml_entry == nullptr)
        {
            log::error("VMM: Could not get page map entry!");
            return;
        }

        pml_entry->setAddr(paddr >> 12);
        pml_entry->setflags(flags | (hugepages ? LargerPages : 0), true);
    }

    void Pagemap::mapMemRange(uint64_t vaddr, uint64_t paddr, uint64_t size, uint64_t flags, bool hugepages)
    {
        for (size_t i = 0; i < size; i += (hugepages ? large_page_size : page_size))
        {
            this->mapMem(vaddr + i, paddr + i, flags, hugepages);
        }
    }

    bool Pagemap::remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags)
    {
        this->lock.lock();

        PDEntry *pml1_entry = this->virt2pte(vaddr_old, false);
        if (pml1_entry == nullptr)
        {
            log::error("VMM: Could not get page map entry!");
            this->lock.unlock();
            return false;
        }

        uint64_t paddr = pml1_entry->getAddr() << 12;
        pml1_entry->value = 0;
        invlpg(vaddr_old);
        this->lock.unlock();

        this->mapMem(vaddr_new, paddr, flags);
        return true;
    }

    bool Pagemap::unmapMem(uint64_t vaddr, bool hugepages)
    {
        lockit(this->lock);

        PDEntry *pml_entry = this->virt2pte(vaddr, false, hugepages);
        if (pml_entry == nullptr)
        {
            log::error("VMM: Could not get page map entry!");
            return false;
        }

        pml_entry->value = 0;
        invlpg(vaddr);
        return true;
    }

    void Pagemap::unmapMemRange(uint64_t vaddr, uint64_t size, bool hugepages)
    {
        for (size_t i = 0; i < size; i += (hugepages ? large_page_size : page_size))
        {
            this->unmapMem(vaddr + i);
        }
    }

    void Pagemap::switchTo()
    {
        write_cr(3, reinterpret_cast<uint64_t>(this->TOPLVL));
    }

    void Pagemap::save()
    {
        this->TOPLVL = reinterpret_cast<PTable*>(read_cr(3));
    }


    Pagemap *newPagemap(bool user)
    {
        Pagemap *pagemap = new Pagemap;
        pagemap->TOPLVL = mm::pmm::alloc<PTable*>();

        if (kernel_pagemap)
        {

            PTable *toplvl = reinterpret_cast<PTable*>(reinterpret_cast<uint64_t>(pagemap->TOPLVL) + hhdm_offset);
            PTable *kerenltoplvl = reinterpret_cast<PTable*>(reinterpret_cast<uint64_t>(kernel_pagemap->TOPLVL) + hhdm_offset);
            for (size_t i = 256; i < 512; i++) toplvl[i] = kerenltoplvl[i];
        }
        else for (size_t i = 256; i < 512; i++) get_next_lvl(pagemap->TOPLVL, i, true);

        if (user == false)
        {
            for (uint64_t i = 0; i < 0x100000000; i += large_page_size)
            {
                pagemap->mapMem(i, i, Present | ReadWrite, true);
                pagemap->mapMem(i + hhdm_offset, i, Present | ReadWrite, true);
            }

            for (size_t i = 0; i < memmap_request.response->entry_count; i++)
            {
                limine_memmap_entry *mmap = memmap_request.response->entries[i];

                uint64_t base = align_down(mmap->base, page_size);
                uint64_t top = align_up(mmap->base + mmap->length, page_size);
                if (top < 0x100000000) continue;

                for (uint64_t t = base; t < top; t += page_size)
                {
                    if (t < 0x100000000) continue;
                    pagemap->mapMem(t, t, Present | ReadWrite);
                    pagemap->mapMem(t + hhdm_offset, t, Present | ReadWrite);
                }
            }
        }

        for (size_t i = 0; i < kernel_file_request.response->kernel_file->size; i += page_size)
        {
            uint64_t paddr = kernel_address_request.response->physical_base + i;
            uint64_t vaddr = kernel_address_request.response->virtual_base + i;
            pagemap->mapMem(vaddr, paddr, Present | ReadWrite | (!kernel_pagemap ? 0 : UserSuper));
        }

        return pagemap;
    }

    PTable *getPagemap()
    {
        return reinterpret_cast<PTable*>(read_cr(3));
    }

    void init()
    {
        kernel_pagemap = newPagemap();
        kernel_pagemap->switchTo();
    }
} // namespace mm::vmm