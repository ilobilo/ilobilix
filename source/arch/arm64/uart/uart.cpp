// Copyright (C) 2022  ilobilo

#if defined(__aarch64__)

#include <arch/arm64/uart/uart.hpp>
#include <drivers/term/term.hpp>
#include <lib/lock.hpp>

namespace arch::arm64::uart
{
    static lock_t lock;

    void printc(char c, void *arg)
    {
        lockit(lock);
    }

    void init()
    {
    }
} // namespace arch::arm64::uart

#endif