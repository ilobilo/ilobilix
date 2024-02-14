// Copyright (C) 2022-2024  ilobilo

#pragma once

#if defined(__x86_64__)
#  include <arch/x86_64/cpu/cpu.hpp>
#elif defined(__aarch64__)
#  include <arch/aarch64/cpu/cpu.hpp>
#else
#  error "Please add #include <arch/*/cpu/cpu.hpp> for this architecture"
#endif