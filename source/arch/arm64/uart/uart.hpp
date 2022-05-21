// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__aarch64__)

namespace arch::arm64::uart
{
    void printc(char c, void *arg = nullptr);

    void init();
} // namespace arch::arm64::uart

#endif