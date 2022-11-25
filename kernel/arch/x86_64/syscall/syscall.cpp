// Copyright (C) 2022  ilobilo

#include <lib/syscall.hpp>
#include <frozen/map.h>
#include <cpu/idt.hpp>

namespace syscall
{
    // TMP start
    uintptr_t log(const char *str)
    {
        return log::println("{}", str);
    }

    uintptr_t exit()
    {
        auto [pid, tid] = proc::pid();
        log::infoln("exiting [{}:{}]", pid, tid);
        proc::exit();
        return 0;
    }
    // TMP end

    constexpr auto map = frozen::make_map<size_t, wrapper>
    ({
        // TMP start
        { 0, wrapper("log", log) },
        { 2, wrapper("exit", exit) },
        // TMP end
    });

    extern "C" void syscall_handler(cpu::registers_t *regs)
    {
        if (auto entry = map.find(regs->rax); entry != map.end())
            regs->rax = cpu::as_user([&entry](auto arg) { return entry->second.run(arg); }, std::array { regs->rdi, regs->rsi, regs->rdx, regs->r10, regs->r8, regs->r9 });
        else
            log::errorln("Unknown syscall {}!", regs->rax);
    }

    void init()
    {
        idt::handlers[idt::INT_SYSCALL].set(syscall_handler);
    }
} // namespace syscall