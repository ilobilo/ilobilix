// Copyright (C) 2024-2025  ilobilo

module system.time;

import frigg;
import arch;
import lib;
import cppstd;

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
        log::info("time: registering clock source '{}'", clock.name);
        clocks.push(&clock);
        log::debug("time: main clock is set to '{}'", (main = clocks.top())->name);
    }

    clock *main_clock()
    {
        return main;
    }

    bool stall_ns(std::uint64_t ns)
    {
        if (main == nullptr)
            return false;

        const auto target = main->ns() + ns;
        while (main->ns() < target)
            arch::pause();

        return true;
    }
} // namespace time