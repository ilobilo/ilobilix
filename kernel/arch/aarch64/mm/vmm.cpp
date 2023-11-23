// Copyright (C) 2022-2023  ilobilo

#include <arch/aarch64/cpu/cpu.hpp>
#include <init/kernel.hpp>
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

        User = (1 << 6),

        RW = (0 << 7),
        RO = (1 << 7),

        Access = (1 << 10),
        NotGlobal = (1 << 11),
        ExecNever = (1UL << 54),

        NonShare = (0b00 << 8),
        OutShare = (0b10 << 8),
        InShare = (0b11 << 8),

        WB = (0b00 << 2) | InShare,
        NC = (0b01 << 2) | OutShare,
        WT = (0b10 << 2) | OutShare
    };

    struct [[gnu::packed]] ttbr { ptentry entries[512] { }; };
    struct ptable
    {
        ttbr *ttbr0;
        ttbr *ttbr1;
    };

    static size_t va_width = 0;
    static size_t psize = 0;

    namespace arch
    {
        uintptr_t pa_mask = 0;
        uintptr_t new_table_flags = Valid | Table;

        void *alloc_ptable()
        {
            return new ttbr;
        }
    } // namespace arch

    bool ptentry::is_valid()
    {
        return this->getflags(Valid);
    }

    bool ptentry::is_large()
    {
        return !this->getflags(Table);
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
        std::unreachable();
    }

    ptentry *pagemap::virt2pte(uint64_t vaddr, bool allocate, uint64_t psize, bool checkll)
    {
        size_t pml5_entry = (vaddr & (0xFULL << 48)) >> 48;
        size_t pml4_entry = (vaddr & (0x1FFULL << 39)) >> 39;
        size_t pml3_entry = (vaddr & (0x1FFULL << 30)) >> 30;
        size_t pml2_entry = (vaddr & (0x1FFULL << 21)) >> 21;
        size_t pml1_entry = (vaddr & (0x1FFULL << 12)) >> 12;

        ttbr *pml4, *pml3, *pml2, *pml1;
        auto half = (vaddr & (1ULL << 63)) ? this->toplvl->ttbr1 : this->toplvl->ttbr0;
        if (half == nullptr)
            return nullptr;

        pml4 = static_cast<ttbr*>(if_max_pgmode(this->get_next_lvl(half->entries[pml5_entry], allocate)) : half);
        if (pml4 == nullptr)
            return nullptr;

        pml3 = static_cast<ttbr*>(this->get_next_lvl(pml4->entries[pml4_entry], allocate, psize));
        if (pml3 == nullptr)
            return nullptr;

        if (psize == this->llpage_size || (checkll && pml3->entries[pml3_entry].is_large()))
            return &pml3->entries[pml3_entry];

        pml2 = static_cast<ttbr*>(this->get_next_lvl(pml3->entries[pml3_entry], allocate, vaddr, this->llpage_size, psize));
        if (pml2 == nullptr)
            return nullptr;

        if (psize == this->lpage_size || (checkll && pml2->entries[pml2_entry].is_large()))
            return &pml2->entries[pml2_entry];

        pml1 = static_cast<ttbr*>(this->get_next_lvl(pml2->entries[pml2_entry], allocate, vaddr, this->lpage_size, psize));
        if (pml1 == nullptr)
            return nullptr;

        return &pml1->entries[pml1_entry];
    }

    uintptr_t pagemap::virt2phys(uintptr_t vaddr, size_t flags)
    {
        std::unique_lock guard(this->lock);

        auto psize = this->get_psize(flags);
        ptentry *pml_entry = this->virt2pte(vaddr, false, psize, true);
        if (pml_entry == nullptr || !pml_entry->getflags(Valid))
            return invalid_addr;

        return pml_entry->getaddr() + (vaddr % psize);
    }

    bool pagemap::map_nolock(uintptr_t vaddr, uintptr_t paddr, size_t flags, caching cache)
    {
        ptentry *pml_entry = this->virt2pte(vaddr, true, this->get_psize(flags), false);
        if (pml_entry == nullptr)
        {
            if (print_errors)
                log::errorln("VMM: Could not get page map entry for address 0x{:X}", vaddr);
            return false;
        }

        auto realflags = flags2arch(flags) | cache2flags(cache);

        pml_entry->reset();
        pml_entry->setaddr(paddr);
        pml_entry->setflags(realflags, true);
        return true;
    }

    bool pagemap::unmap_nolock(uintptr_t vaddr, size_t flags)
    {
        ptentry *pml_entry = this->virt2pte(vaddr, false, this->get_psize(flags), true);
        if (pml_entry == nullptr)
        {
            if (print_errors)
                log::errorln("VMM: Could not get page map entry for address 0x{:X}", vaddr);
            return false;
        }

        pml_entry->reset();
        cpu::invlpg(vaddr);
        return true;
    }

    bool pagemap::setflags_nolock(uintptr_t vaddr, size_t flags, caching cache)
    {
        ptentry *pml_entry = this->virt2pte(vaddr, false, this->get_psize(flags), true);
        if (pml_entry == nullptr)
        {
            if (print_errors)
                log::errorln("VMM: Could not get page map entry for address 0x{:X}", vaddr);
            return false;
        }

        auto realflags = flags2arch(flags) | cache2flags(cache);
        auto addr = pml_entry->getaddr();

        pml_entry->reset();
        pml_entry->setaddr(addr);
        pml_entry->setflags(realflags, true);
        return true;
    }

    void pagemap::load(bool hh)
    {
        write_ttbr_el1(0, fromhh(reinterpret_cast<uint64_t>(this->toplvl->ttbr0)));
        if (hh == true)
            write_ttbr_el1(1, fromhh(reinterpret_cast<uint64_t>(this->toplvl->ttbr1)));
    }

    void pagemap::save()
    {
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

        PANIC("VMM: Unknown VA width {}", va_width);
    }

    uintptr_t flags2arch(size_t flags)
    {
        uintptr_t ret = Valid | Access;
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

    std::pair<size_t, caching> arch2flags(uintptr_t flags, bool lpages)
    {
        size_t ret1 = 0;
        if (flags & Valid)
            ret1 |= read;
        if (!(flags & RO))
            ret1 |= write;
        if (!(flags & ExecNever))
            ret1 |= exec;
        if (flags & User)
            ret1 |= user;
        if (!(flags & NotGlobal))
            ret1 |= global;

        caching ret2;
        if (flags & NC)
            ret2 = uncachable;
        if (flags & WT)
            ret2 = write_through;
        if (flags & WB)
            ret2 = write_back;

        return { ret1, ret2 };
    }

    void arch_destroy_pmap(pagemap *pmap)
    {
        // TODO
    }

    enum paranges : uint64_t
    {
        bits32 = 0b0000,
        bits36 = 0b0001,
        bits40 = 0b0010,
        bits42 = 0b0011,
        bits44 = 0b0100,
        bits48 = 0b0101,
        bits52 = 0b0110 // when lpa or lpa2
    };

    union mmfr0
    {
        struct [[gnu::packed]] {
            uint64_t parange : 4;   // physical address range
            uint64_t asidbits : 4;  // 0b0000: 8 bits, 0b0010: 16 bits
            uint64_t bigend : 4;    // 0b0001: mixed-endian support
            uint64_t snsmem : 4;    // 0b0001: secure memory support
            uint64_t bigendel0 : 4; // 0b0001: el0 mixed-endian support
            uint64_t tgran16 : 4;   // 0b0000: no 16kb, 0b0001: 16kb, 0b0010: 16kb with pa52 lpa2
            uint64_t tgran64 : 4;   // 0b0000: 64kb, 0b1111: no 64kb
            uint64_t tgran4 : 4;    // 0b0000: 4kb, 0b0001: 4kb with pa52 lpa2, 0b1111 no 4kb
            uint64_t tgran16_2 : 4; // 0b0001: no st2 16kb, 0b0010: st2 16kb, 0b0011: st2 16kb with pa52 lpa2
            uint64_t tgran64_2 : 4; // 0b0001: no st2 64kb, 0b0010: st2 64kb
            uint64_t tgran4_2 : 4;  // 0b0001: no st2 4kb, 0b0010: st2 4kb, 0b0011: st2 4kb with pa52 lpa2
            uint64_t exs : 4;       // 0b0000: no, 0b0001: disable context-sync exception
            uint64_t rsv0 : 8;      // reserved
            uint64_t fgt : 4;       // 0b0001: fine-frained traps
            uint64_t ecv : 4;       // 0b0001: enchanced counter virt
        };
        uint64_t raw;
    };
    static_assert(sizeof(mmfr0) == 8);

    union mmfr2
    {
        struct [[gnu::packed]] {
            uint64_t cnp : 4;
            uint64_t uao : 4;
            uint64_t lsm : 4;
            uint64_t iesb : 4;
            uint64_t varange : 4; // 0b0000: 48bit va, 0b0001: 52bit va with 64kb
            uint64_t ccidx : 4;
            uint64_t nv : 4;
            uint64_t st : 4;
            uint64_t at : 4;
            uint64_t ids : 4;
            uint64_t fwb : 4;
            uint64_t rsv0 : 4;
            uint64_t ttl : 4;
            uint64_t bbm : 4;
            uint64_t evt : 4;
            uint64_t e0pd : 4;
        };
        uint64_t raw;
    };
    static_assert(sizeof(mmfr2) == 8);

    static constexpr size_t psize_4kib = 0x1000;
    static constexpr size_t psize_16kib = 0x4000;
    static constexpr size_t psize_64kib = 0x10000;

    void arch_init()
    {
        mmfr0 aa64mmfr0;
        mmfr2 aa64mmfr2;
        uint64_t tcr_el1 = 0;

        asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r"(aa64mmfr0.raw));
        asm volatile ("mrs %0, id_aa64mmfr2_el1" : "=r"(aa64mmfr2.raw));
        asm volatile ("mrs %0, tcr_el1" : "=r"(tcr_el1));

        bool feat_lpa = (aa64mmfr0.parange == paranges::bits52);
        bool feat_lva = (aa64mmfr2.varange == 0b0001);

        if (aa64mmfr0.tgran4 != 0b1111)
            psize = psize_4kib;
        else if (aa64mmfr0.tgran16 != 0b0000)
            psize = psize_16kib;
        else if (aa64mmfr0.tgran64 == 0b0000)
            psize = psize_64kib;
        else
            PANIC("VMM: Unknown page size!");

        if (paging_mode == LIMINE_PAGING_MODE_MAX && (
                ((tcr_el1 << 59) & 1) == 1 && feat_lpa == true && (
                    (psize == psize_64kib && feat_lva == true) ||
                    (psize == psize_16kib && aa64mmfr0.tgran16 == 0b0010) ||
                    (psize == psize_4kib && aa64mmfr0.tgran4 == 0b0001)
                )
            )
        )    va_width = 52;
        else va_width = 48;

        // 48-Bit
        // 0b0000000000000000111111111111111111111111111111111111000000000000 : 0x0000FFFFFFFFF000 -> 4kib pages
        // 0b0000000000000000111111111111111111111111111111111100000000000000 : 0x0000FFFFFFFFC000 -> 16kib pages
        // 0b0000000000000000111111111111111111111111111111110000000000000000 : 0x0000FFFFFFFF0000 -> 64kib pages

        // 52-Bit
        // 0b0000000000000011111111111111111111111111111111111111000000000000 : 0x0003FFFFFFFFF000 -> 4kib pages
        // 0b0000000000000011111111111111111111111111111111111100000000000000 : 0x0003FFFFFFFFC000 -> 16kib pages
        // 0b0000000000000000111111111111111111111111111111110000000000000000 : 0x0000FFFFFFFF0000 -> 64kib pages

        static constexpr auto map = frozen::make_map<std::tuple<size_t, size_t>, uintptr_t>
        ({
            { { psize_4kib, 48 }, 0x0000FFFFFFFFF000 },
            { { psize_16kib, 48 }, 0x0000FFFFFFFFC000 },
            { { psize_64kib, 48 }, 0x0000FFFFFFFF0000 },

            { { psize_4kib, 52 }, 0x0003FFFFFFFFF000 },
            { { psize_16kib, 52 }, 0x0003FFFFFFFFC000 },
            { { psize_64kib, 52 }, 0x0000FFFFFFFF0000 }
        });
        arch::pa_mask = map.at({ psize, va_width });
    }
} // namespace vmm