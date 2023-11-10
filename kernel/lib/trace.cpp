// Copyright (C) 2022-2023  ilobilo

#include <drivers/elf.hpp>
#include <lib/trace.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <cxxabi.h>

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
            auto [entry, offset, is_mod] = elf::syms::lookup(ip, STT_FUNC);
            if (entry == elf::syms::empty_sym)
                return false;

            std::string_view name = abi::__cxa_demangle(entry.name.data(), nullptr, nullptr, nullptr) ?: entry.name;
            log::println("{}  [0x{:016X}] ({}) <{}+0x{:X}>", prefix, entry.addr, is_mod ? "Module" : "Kernel", name, offset);

            return name != "int_handler" && name != "syscall_handler";
        };

        log::println("{}Stacktrace:", prefix);
        if (fip != 0)
            print_name(fip);

        for (size_t i = 0; i < 10; i++)
        {
            if (frame == nullptr || frame->ip == 0)
                break;

            if (print_name(frame->ip) == false)
                break;

            frame = frame->next;
        }
    }
} // namespace trace