// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__aarch64__)

#include <limine.h>

namespace arch::arm64::smp
{
    void cpu_init(limine_smp_info *cpu);
} // namespace arch::arm64::smp

#endif