// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__x86_64__) || defined(_M_X64)

namespace arch::x86_64::uart
{
    enum COMS
    {
        COM1 = 0x3F8,
        COM2 = 0x2F8,
        COM3 = 0x3E8,
        COM4 = 0x2E8
    };

    void printc(char c, void *arg = nullptr);
    int print(COMS com, const char *fmt, ...);

    void init();
} // namespace arch::x86_64::uart

#endif