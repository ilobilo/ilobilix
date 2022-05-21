// Copyright (C) 2022  ilobilo

#if defined(__aarch64__)

#include <lib/log.hpp>
#include <smp/smp.hpp>

namespace arch::arm64
{
    void init()
    {
        log::info("Initialising SMP...");
        smp::init();
    }
} // namespace arch::arm64

#endif