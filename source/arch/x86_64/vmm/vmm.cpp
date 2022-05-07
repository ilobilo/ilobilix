// Copyright (C) 2022  ilobilo

#if defined(__x86_64__) || defined(_M_X64)

#include <arch/x86_64/vmm/vmm.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <mm/pmm/pmm.hpp>
#include <mm/vmm/vmm.hpp>
#include <arch/arch.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <main.hpp>

namespace arch::x86_64::vmm
{
    bool lvl5 = LVL5_PAGING;

    static PTable *get_next_lvl(PTable *curr_lvl, size_t entry, bool allocate = true)
    {
        PTable *ret = nullptr;
        if (curr_lvl->entries[entry].getflags(Present))
        {
            ret = reinterpret_cast<PTable*>(curr_lvl->entries[entry].getAddr() << 12);
        }
        else if (allocate == true)
        {
            ret = mm::pmm::alloc<PTable*>();
            curr_lvl->entries[entry].setAddr(reinterpret_cast<uint64_t>(ret) >> 12);
            curr_lvl->entries[entry].setflags(Present | Write | UserSuper, true);
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
            pml5 = this->toplvl;
            if (pml5 == nullptr) return nullptr;

            pml4 = get_next_lvl(pml5, pml5_entry, allocate);
        }
        else
        {
            pml5 = nullptr;
            pml4 = this->toplvl;
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

    bool Pagemap::mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags, bool hugepages)
    {
        lockit(this->lock);

        PDEntry *pml_entry = this->virt2pte(vaddr, true, hugepages);
        if (pml_entry == nullptr)
        {
            log::error("VMM: Could not get page map entry!");
            return false;
        }

        pml_entry->setAddr(paddr >> 12);
        pml_entry->setflags(flags | (hugepages ? LargerPages : 0), true);
        return true;
    }

    bool Pagemap::remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags, bool hugepages)
    {
        this->lock.lock();

        PDEntry *pml1_entry = this->virt2pte(vaddr_old, false, hugepages);
        if (pml1_entry == nullptr)
        {
            log::error("VMM: Could not get page map entry!");
            this->lock.unlock();
            return false;
        }

        uint64_t paddr = pml1_entry->getAddr() << 12;
        pml1_entry->value = 0;
        cpu::invlpg(vaddr_old);
        this->lock.unlock();

        this->mapMem(vaddr_new, paddr, flags, hugepages);
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
        cpu::invlpg(vaddr);
        return true;
    }

    void Pagemap::switchTo()
    {
        write_cr(3, reinterpret_cast<uint64_t>(this->toplvl));
    }

    void Pagemap::save()
    {
        this->toplvl = reinterpret_cast<PTable*>(read_cr(3));
    }

    Pagemap::Pagemap()
    {
        this->large_page_size = 0x200000;
        this->page_size = 0x1000;

        this->toplvl = mm::pmm::alloc<PTable*>();
    }

    uint64_t getPagemap()
    {
        return read_cr(3);
    }
} // namespace arch::x86_64::vmm

#endif