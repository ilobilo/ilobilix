// Copyright (C) 2024  ilobilo

export module x86_64.drivers.timers.kvm;

import drivers.timers.acpipm;
import system.cpu.self;
import system.time;
import system.cpu;
import arch;
import lib;
import std;

namespace x86_64::timers::tsc
{
    extern "C++" std::uint64_t rdtsc();
} // x86_64::timers::tsc

namespace x86_64::timers::kvm
{
    namespace
    {
        struct [[gnu::packed]] kvmclock_info
        {
            std::uint32_t version;
            std::uint32_t pad0;
            std::uint64_t tsc_timestamp;
            std::uint64_t system_time;
            std::uint32_t tsc_to_system_mul;
            std::int8_t tsc_shift;
            std::uint8_t flags;
            std::uint8_t pad[2];
        };
        std::int64_t offset = 0;
    } // namespace

    export bool supported()
    {
        static const auto cached = [] -> bool
        {
            bool kvmclock = false;
            if (const auto base = cpu::kvm_base(); base)
            {
                std::uint32_t a, b, c, d;
                kvmclock = cpu::id(base + 1, 0, a, b, c, d) && (a & (1 << 3));
            }
            log::info("KVM clock supported: {}", kvmclock);
            return kvmclock;
        } ();
        return cached;
    }

    export std::uint64_t time_ns()
    {
        auto self = cpu::self()->arch.kvm;
        volatile auto pvclock = static_cast<kvmclock_info *>(self.pvclock);

        while (pvclock->version % 2)
            arch::pause();

        uint128_t time = tsc::rdtsc() - pvclock->tsc_timestamp;
        if (pvclock->tsc_shift >= 0)
            time <<= pvclock->tsc_shift;
        else
            time >>= -pvclock->tsc_shift;
        time = (time * pvclock->tsc_to_system_mul) >> 32;
        time = time + pvclock->system_time;

        return time - offset;
    }

    export std::uint64_t tsc_freq()
    {
        volatile auto self = static_cast<kvmclock_info *>(cpu::self()->arch.kvm.pvclock);

        std::uint64_t freq = (1'000'000'000ull << 32) / self->tsc_to_system_mul;
        if (self->tsc_shift < 0)
            freq <<= -self->tsc_shift;
        else
            freq >>= self->tsc_shift;
        return freq;
    }

    time::clock clock { "kvm", 100, time_ns };
    export void init()
    {
        if (!supported())
            return;

        auto &self = cpu::self()->arch.kvm;
        self.pvclock = reinterpret_cast<void *>(new kvmclock_info);
        cpu::msr::write(0x4B564D01, reinterpret_cast<std::uint64_t>(lib::fromhh(self.pvclock)) | 1);

        [[maybe_unused]]
        static const auto cached = []
        {
            if (auto clock = time::main_clock(); clock)
                offset = time_ns() - clock->ns();

            time::register_clock(clock);
            return true;
        } ();
    }
} // export namespace x86_64::timers::kvm