// Copyright (C) 2022  ilobilo

#if defined(__x86_64__) || defined(_M_X64)
#include <arch/x86_64/x86_64.hpp>
#elif defined(__aarch64__) || defined(_M_ARM64)
#include <arch/arm64/arm64.hpp>
#endif
#include <arch/arch.hpp>
#include <lib/log.hpp>

namespace arch
{
    Pagemap *kernel_pagemap;

    void init()
    {
        #if defined(__x86_64__) || defined(_M_X64)
        x86_64::init();
        #elif defined(__aarch64__) || defined(_M_ARM64)
        arm64::init();
        #endif
    }
} // namespace arch