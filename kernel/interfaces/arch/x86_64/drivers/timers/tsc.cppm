// Copyright (C) 2024-2025  ilobilo

export module x86_64.drivers.timers.tsc;

import system.cpu.self;
import lib;
import cppstd;

export namespace x86_64::timers::tsc
{
    bool supported();

    lib::freqfrac frequency();

    extern "C++" std::uint64_t rdtsc();
    std::uint64_t time_ns();

    void init();
    void finalise();
} // export namespace x86_64::timers::tsc