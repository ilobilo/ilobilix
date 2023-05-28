// Copyright (C) 2022-2023  ilobilo

#include <drivers/smp.hpp>
#include <arch/arch.hpp>
#include <utility>

namespace arch
{
    [[noreturn]] void halt(bool ints)
    {
        if (ints == true)
        {
            while (true)
                asm volatile ("wfi");
        }
        else
        {
            while (true)
                asm volatile ("msr daifclr, #0b1111; wfi");
            std::unreachable();
        }
    }

    void halt_others()
    {
        // TODO
    }

    void dump_regs(cpu::registers_t *regs, const char *prefix)
    {
        // TODO
    }

    void wfi()
    {
        asm volatile ("wfi");
    }

    void pause()
    {
        asm volatile ("yield");
    }

    void int_toggle(bool on)
    {
        if (on == true)
            asm volatile ("msr daifclr, #0b1111");
        else
            asm volatile ("msr daifset, #0b1111");
    }

    bool int_status()
    {
        uint64_t val = 0;
        asm volatile ("mrs %0, daif" : "=r"(val));
        return val == 0;
    }

    std::optional<uint64_t> time_ns()
    {
        return std::nullopt;
    }

    uint64_t epoch()
    {
        return 0;
    }

    // TODO
    [[noreturn]] void shutdown();
    [[noreturn]] void reboot();

    void init()
    {
        smp::bsp_init();

        smp::init();
    }

    void late_init()
    {
    }
} // namespace arch