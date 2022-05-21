// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__x86_64__)

#include <limine.h>

namespace arch::x86_64::smp
{
    void cpu_init(limine_smp_info *cpu);
} // namespace arch::x86_64::smp

#endif