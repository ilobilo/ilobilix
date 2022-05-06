// Copyright (C) 2024  ilobilo

module system.time;

import frigg;
import arch;
import lib;
import std;

namespace time
{
    namespace
    {
        struct higher_priority
        {
            constexpr bool operator()(const clock *lhs, const clock *rhs) const
            {
                return lhs->priority < rhs->priority;
            }
        };

        frg::pairing_heap<
            clock,
            frg::locate_member<
                clock,
                frg::pairing_heap_hook<clock>,
                &clock::hook
            >,
            higher_priority
        > clocks;
        clock *main = nullptr;
    } // namespace

    void register_clock(clock &clock)
    {
        log::debug("Registering clock '{}'", clock.name);
        clocks.push(&clock);
        log::debug("Main clock is set to '{}'", (main = clocks.top())->name);
    }

    clock *main_clock()
    {
        return main;
    }

    void stall_ns(std::uint64_t ns)
    {
        if (!main)
            return;

        auto target = main->ns() + ns;
        while (main->ns() < target)
            arch::pause();
    }

    void sleep_ns(std::size_t ns)
    {
        // TODO: propa eep
        stall_ns(ns);
    }
} // namespace time