// Copyright (C) 2022  ilobilo

#include <drivers/elf.hpp>
#include <lib/trace.hpp>
#include <lib/log.hpp>

namespace trace
{
    void print(uintptr_t bp, uintptr_t fip, const char *prefix)
    {
        if (bp == 0)
            bp = reinterpret_cast<uintptr_t>(__builtin_frame_address(0));

        auto frame = reinterpret_cast<stackframe*>(bp);
        if (frame == nullptr)
            return;

        auto print_name = [&prefix](uintptr_t ip)
        {
            auto [entry, offset] = elf::syms::lookup(ip, STT_FUNC);
            if (entry != elf::syms::empty_sym)
                log::println("{}  [0x{:016X}] <{}+0x{:X}>", prefix, entry.addr, entry.name, offset);
        };

        log::println("{}Stacktrace:", prefix);
        if (fip != 0)
            print_name(fip);

        // while (true)
        for (size_t i = 0; i < 10; i++)
        {
            if (frame == nullptr || frame->ip == 0)
                break;

            print_name(frame->ip);
            frame = frame->next;
        }
    }
} // namespace trace