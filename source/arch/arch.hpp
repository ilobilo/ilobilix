// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__x86_64__) || defined(_M_X64)
#include <arch/x86_64/vmm/vmm.hpp>
#elif defined(__aarch64__) || defined(_M_ARM64)
#include <arch/arm64/vmm/vmm.hpp>
#endif

namespace arch
{
    #if defined(__x86_64__) || defined(_M_X64)
    using Pagemap = x86_64::vmm::Pagemap;
    #elif defined(__aarch64__) || defined(_M_ARM64)
    using Pagemap = arm64::vmm::Pagemap;
    #endif

    extern Pagemap *kernel_pagemap;

    void init();
} // namespace arch