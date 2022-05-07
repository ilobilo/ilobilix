// Copyright (C) 2022  ilobilo

#if defined(__aarch64__) || defined(_M_ARM64)

#include <arch/arm64/vmm/vmm.hpp>
#include <mm/pmm/pmm.hpp>
#include <arch/arch.hpp>
#include <lib/panic.hpp>
#include <lib/log.hpp>

namespace arch::arm64::vmm
{
    static void setflags(uint64_t &flags)
    {
        if (flags & NoExec) flags &= ~NoExec;
        else flags |= NoExec;
        if (flags & UserSuper) flags &= ~UserSuper;
    }

    bool Pagemap::mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags, bool hugepages)
    {
        lockit(this->lock);
        setflags(flags);

        // TODO

        return true;
    }

    bool Pagemap::remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags, bool hugepages)
    {
        this->lock.lock();
        setflags(flags);

        // TODO

        return true;
    }

    bool Pagemap::unmapMem(uint64_t vaddr, bool hugepages)
    {
        lockit(this->lock);

        // TODO

        return true;
    }

    void Pagemap::switchTo()
    {
        asm volatile ("MSR %[ttbr0], TTBR0_EL1; MSR %[ttbr1], TTBR1_EL1" :: [ttbr0]"r"(this->ttbr0), [ttbr1]"r"(this->ttbr1) : "memory");
    }

    void Pagemap::save()
    {
        asm volatile ("MSR TTBR0_EL1, %[ttbr0]; MSR TTBR1_EL1, %[ttbr1]" : [ttbr0]"=r"(this->ttbr0), [ttbr1]"=r"(this->ttbr1) :: "memory");
    }

    Pagemap::Pagemap()
    {
        uint64_t aa64mmfr0 = 0;
        asm volatile ("MRS %[aa64mmfr0], ID_AA64MMFR0_EL1" : [aa64mmfr0]"=r"(aa64mmfr0));

        if (((aa64mmfr0 >> 28) & 0x0F) == 0b0000) this->page_size = 0x1000;
        else if (((aa64mmfr0 >> 20) & 0x0F) == 0b0001) this->page_size = 0x4000;
        else if (((aa64mmfr0 >> 24) & 0x0F) == 0b0000) this->page_size = 0x10000;
        else panic("VMM: unknown page size!");

        this->large_page_size = this->page_size; // ?

        this->ttbr0 = mm::pmm::alloc(this->page_size / 0x1000);
        this->ttbr1 = mm::pmm::alloc(this->page_size / 0x1000);
    }
} // namespace arch::arm64::vmm

#endif