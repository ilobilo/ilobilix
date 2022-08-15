// Copyright (C) 2022  ilobilo

#include <drivers/elf.hpp>
#include <lib/trace.hpp>
#include <lib/log.hpp>

namespace trace
{
    void print(uintptr_t bp, int (*func)(const char *, ...))
    {
        if (func == nullptr)
            func = log::println;

        auto frame = reinterpret_cast<stackframe*>(bp);
        if (frame == nullptr)
            return;

        func("Stacktrace:");

        while (true)
        {
            if (frame == nullptr || frame->ip == 0)
                break;

            auto [entry, offset] = elf::syms::lookup(frame->ip, STT_FUNC);
            if (entry != elf::syms::empty_sym)
                func("  [0x%.16lX] <%.*s+0x%lX>", entry.addr, entry.name.length(), entry.name.data(), offset);

            frame = frame->next;
        }
    }
} // namespace trace