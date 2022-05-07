// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__aarch64__) || defined(_M_ARM64)

#include <lib/lock.hpp>
#include <cstdint>

namespace arch::arm64::vmm
{
    enum flags
    {
        Present = (1 << 0),
        Write = (1 << 1),
        NoExec = (1 << 2),
        UserSuper = (1 << 3)
    };

    struct Pagemap
    {
        void *ttbr0 = nullptr;
        void *ttbr1 = nullptr;
        uint64_t large_page_size = 0;
        uint64_t page_size = 0;
        lock_t lock;

        bool mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags = (Present | Write), bool hugepages = false);
        bool remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags = (Present | Write), bool hugepages = false);
        bool unmapMem(uint64_t vaddr, bool hugepages = false);

        void switchTo();
        void save();

        Pagemap();
    };
} // namespace arch::arm64::vmm

#endif