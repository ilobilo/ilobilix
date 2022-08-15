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

    void print(uintptr_t bp, int (*func)(const char *, ...) = nullptr);
} // namespace trace