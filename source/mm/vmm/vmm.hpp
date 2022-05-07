// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__x86_64__) || defined(_M_X64)
#include <arch/x86_64/vmm/vmm.hpp>
#elif defined(__aarch64__) || defined(_M_ARM64)
#include <arch/arm64/vmm/vmm.hpp>
#endif

namespace mm::vmm
{
    #if defined(__x86_64__) || defined(_M_X64)
    using Pagemap = arch::x86_64::vmm::Pagemap;
    using flags = arch::x86_64::vmm::flags;
    #elif defined(__aarch64__) || defined(_M_ARM64)
    using Pagemap = arch::arm64::vmm::Pagemap;
    using flags = arch::arm64::vmm::flags;
    #endif

    extern Pagemap *kernel_pagemap;

    Pagemap *newPagemap(bool user = false);
    void init();
} // namespace mm::vmm