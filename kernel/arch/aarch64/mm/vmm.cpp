// Copyright (C) 2022  ilobilo

#include <arch/aarch64/cpu/cpu.hpp>
#include <lib/panic.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

#include <frozen/map.h>

namespace vmm
{
    enum
    {
        Valid = (1 << 0),
        Table = (1 << 1),
        Block = (0 << 1),
        Page = (1 << 1),

        ExecNever = (1UL << 54),
        NotGlobal = (1 << 11),
        Access = (1 << 10),
        NonShare = (0b00 << 8),
        OutShare = (0b10 << 8),
        InShare = (0b11 << 8),
        RW = (0 << 7),
        RO = (1 << 7),
        User = (1 << 6),
        WB = (0 << 2),
        NC = (1 << 2),
        WT = (2 << 2),
    };

    struct [[gnu::packed]] ttbr { ptentry entries[512]; };
    struct ptable
    {
        ttbr *ttbr0;
        ttbr *ttbr1;
    };

    // TODO: Actually working 16 kib and 64 kib pages

    static constexpr size_t psize_4kib = 0x1000;
    static constexpr size_t psize_16kib = 0x4000;
    static constexpr size_t psize_64kib = 0x10000;

    // 48-Bit
    // 0b0000000000000000111111111111111111111111111111111111000000000000 : 0x0000FFFFFFFFF000 -> 4kib pages
    // 0b0000000000000000111111111111111111111111111111111100000000000000 : 0x0000FFFFFFFFC000 -> 16kib pages
    // 0b0000000000000000111111111111111111111111111111110000000000000000 : 0x0000FFFFFFFF0000 -> 64kib pages

    // 52-Bit
    // 0b0000000000000011111111111111111111111111111111111111000000000000 : 0x0003FFFFFFFFF000 -> 4kib pages
    // 0b0000000000000011111111111111111111111111111111111100000000000000 : 0x0003FFFFFFFFC000 -> 16kib pages
    // 0b0000000000000000111111111111111111111111111111110000000000000000 : 0x0000FFFFFFFF0000 -> 64kib pages

    // psize and va_width to pa_mask
    constexpr auto map = frozen::make_map<std::tuple<size_t, size_t>, uintptr_t>
    ({
        { { psize_4kib, 48 }, 0x0000FFFFFFFFF000 },
        { { psize_4kib, 52 }, 0x0003FFFFFFFFF000 },
        { { psize_16kib, 48 }, 0x0000FFFFFFFFC000 },
        { { psize_16kib, 52 }, 0x0003FFFFFFFFC000 },
        { { psize_64kib, 48 }, 0x0000FFFFFFFF0000 },
        { { psize_64kib, 52 }, 0x0000FFFFFFFF0000 }
    });

    static size_t va_width = 0;
    static size_t psize = 0;

    uintptr_t pa_mask = 0;

    static ttbr *get_next_lvl(ttbr *curr_lvl, size_t entry, bool allocate = true)
    {
        if (curr_lvl == nullptr)
            return nullptr;

        ttbr *ret = nullptr;

        if (curr_lvl->entries[entry].getflags(Valid | Table))
            ret = reinterpret_cast<ttbr*>(tohh(curr_lvl->entries[entry].getaddr()));
        else if (allocate == true)
        {
            ret = new ttbr;
            curr_lvl->entries[entry].setaddr(fromhh(reinterpret_cast<uint64_t>(ret)));
            curr_lvl->entries[entry].setflags(Valid | Table, true);
        }
        return ret;
    }

    static uint64_t cache2flags(caching cache)
    {
        switch (cache)
        {
            case uncachable:
                return NC;
            case write_through:
                return WT;
            default:
                return WB;
        }
        __builtin_unreachable();
    }

    ptentry *pagemap::virt2pte(uint64_t vaddr, bool allocate, uint64_t psize)
    {
        size_t pml5_entry = (vaddr & (0xFULL << 48)) >> 48;
        size_t pml4_entry = (vaddr & (0x1FFULL << 39)) >> 39;
        size_t pml3_entry = (vaddr & (0x1FFULL << 30)) >> 30;
        size_t pml2_entry = (vaddr & (0x1FFULL << 21)) >> 21;
        size_t pml1_entry = (vaddr & (0x1FFULL << 12)) >> 12;

        ttbr *pml4, *pml3, *pml2, *pml1;
        auto half = (vaddr & (1ULL << 63)) ? this->toplvl->ttbr1 : this->toplvl->ttbr0;

        pml4 = lvl5 ? get_next_lvl(half, pml5_entry, allocate) : half;
        if (pml4 == nullptr)
            return nullptr;

        pml3 = get_next_lvl(pml4, pml4_entry, allocate);
        if (pml3 == nullptr)
            return nullptr;

        if (psize == this->llpage_size)
            return &pml3->entries[pml3_entry];

        pml2 = get_next_lvl(pml3, pml3_entry, allocate);
        if (pml2 == nullptr)
            return nullptr;

        if (psize == this->lpage_size)
            return &pml2->entries[pml2_entry];

        pml1 = get_next_lvl(pml2, pml2_entry, allocate);
        if (pml1 == nullptr)
            return nullptr;

        return &pml1->entries[pml1_entry];
    }

    uintptr_t pagemap::virt2phys(uintptr_t vaddr, size_t flags)
    {
        lockit(this->lock);

        ptentry *pml_entry = this->virt2pte(vaddr, false, this->get_psize(flags));
        if (pml_entry == nullptr || !pml_entry->getflags(Valid))
            return invalid_addr;

        return pml_entry->getaddr();
    }

    bool pagemap::map(uintptr_t vaddr, uintptr_t paddr, size_t flags, caching cache)
    {
        lockit(this->lock);

        ptentry *pml_entry = this->virt2pte(vaddr, true, this->get_psize(flags));
        if (pml_entry == nullptr)
        {
            log::errorln("VMM: Could not get page map entry!");
            return false;
        }

        auto realflags = flags2arch(flags) | cache2flags(cache);

        pml_entry->value = 0;
        pml_entry->setaddr(paddr);
        pml_entry->setflags(realflags, true);
        return true;
    }

    bool pagemap::unmap(uintptr_t vaddr, size_t flags)
    {
        lockit(this->lock);

        ptentry *pml_entry = this->virt2pte(vaddr, false, this->get_psize(flags));
        if (pml_entry == nullptr)
        {
            log::errorln("VMM: Could not get page map entry!");
            return false;
        }

        pml_entry->value = 0;
        cpu::invlpg(vaddr);
        return true;
    }

    bool pagemap::setflags(uintptr_t vaddr, size_t flags, caching cache)
    {
        lockit(this->lock);

        ptentry *pml_entry = this->virt2pte(vaddr, false, this->get_psize(flags));
        if (pml_entry == nullptr)
        {
            log::errorln("VMM: Could not get page map entry!");
            return false;
        }

        auto realflags = flags2arch(flags) | cache2flags(cache);
        auto addr = pml_entry->getaddr();

        pml_entry->value = 0;
        pml_entry->setaddr(addr);
        pml_entry->setflags(realflags, true);
        return true;
    }

    void pagemap::load()
    {
        lockit(this->lock);
        write_ttbr_el1(0, fromhh(reinterpret_cast<uint64_t>(this->toplvl->ttbr0)));
        write_ttbr_el1(1, fromhh(reinterpret_cast<uint64_t>(this->toplvl->ttbr1)));
    }

    void pagemap::save()
    {
        lockit(this->lock);
        this->toplvl->ttbr0 = reinterpret_cast<ttbr*>(tohh(read_ttbr_el1(0)));
        this->toplvl->ttbr1 = reinterpret_cast<ttbr*>(tohh(read_ttbr_el1(1)));
    }

    pagemap::pagemap() : toplvl(new ptable { new ttbr, nullptr })
    {
        this->llpage_size = psize * 512 * 512;
        this->lpage_size = psize * 512;
        this->page_size = psize;

        if (kernel_pagemap == nullptr)
            this->toplvl->ttbr1 = new ttbr;
        else
            this->toplvl->ttbr1 = kernel_pagemap->toplvl->ttbr1;
    }

    bool is_canonical(uintptr_t addr)
    {
        if (va_width == 52)
            return (addr <= 0x000FFFFFFFFFFFFFULL) || (addr >= 0xFFF0000000000000ULL);
        else if (va_width == 48)
            return (addr <= 0x0000FFFFFFFFFFFFULL) || (addr >= 0xFFFF000000000000ULL);

        PANIC("VMM: Unknown VA width!");
    }

    uintptr_t flags2arch(size_t flags)
    {
        uintptr_t ret = Valid | InShare | Access;
        if (!(flags & write))
            ret |= RO;
        if (!(flags & exec))
            ret |= ExecNever;
        if (flags & user)
            ret |= User;
        if (!(flags & global))
            ret |= NotGlobal;
        if (!islpage(flags))
            ret |= Page;
        return ret;
    }

    void arch_destroy_pmap(pagemap *pmap)
    {
        // TODO
    }

    void arch_init()
    {
        // TODO: Is anything here correct?

        uint64_t aa64mmfr0 = 0;
        asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r"(aa64mmfr0));

        uint64_t aa64mmfr2 = 0;
        asm volatile ("mrs %0, id_aa64mmfr2_el1" : "=r"(aa64mmfr2));

        uint64_t tcr_el1 = 0;
        asm volatile ("mrs %0, tcr_el1" : "=r"(tcr_el1));

        if (((aa64mmfr0 << 28) & 0b1111) != 0b1111)
            psize = psize_4kib;
        else if (((aa64mmfr0 << 20) & 0b1111) != 0b0000)
            psize = psize_16kib;
        else if (((aa64mmfr0 << 24) & 0b1111) == 0b0000)
            psize = psize_64kib;
        else PANIC("VMM: Unknown page size!");

        // Especially here

        bool feat_lpa = (aa64mmfr0 & 0b1111) == 0b0110;
        bool feat_lva = ((aa64mmfr2 << 16) & 0b1111) == 0b0001;

        if ((psize == psize_64kib && feat_lva == true && feat_lpa == true) ||
            (((tcr_el1 << 59) & 1) == 1 && feat_lpa == true && (
            (psize == psize_16kib && ((aa64mmfr0 << 20) & 0b1111) == 0b0010) ||
            (psize == psize_4kib  && ((aa64mmfr0 << 28) & 0b1111) == 0b0001))))
            va_width = 52;
        else va_width = 48;

        pa_mask = map.at({ psize, va_width });

        // log::infoln("VMM: page size: {:#x}, va width: {}", psize, va_width);
    }
} // namespace vmm