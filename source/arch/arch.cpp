// Copyright (C) 2022  ilobilo

#if defined(__x86_64__)
#include <arch/x86_64/x86_64.hpp>
#elif defined(__aarch64__)
#include <arch/arm64/arm64.hpp>
#endif

namespace arch
{
    void init()
    {
        #if defined(__x86_64__)
        x86_64::init();
        #elif defined(__aarch64__)
        arm64::init();
        #endif
    }
} // namespace arch