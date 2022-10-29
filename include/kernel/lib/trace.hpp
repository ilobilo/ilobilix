// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

namespace trace
{
    struct stackframe
    {
        stackframe *next;
        uintptr_t ip;
    };

    void print(uintptr_t bp = 0, uintptr_t fip = 0, const char *prefix = "");
} // namespace trace