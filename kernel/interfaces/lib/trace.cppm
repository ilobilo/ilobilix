// Copyright (C) 2024-2025  ilobilo

export module lib:trace;

import :log;
import cppstd;

export namespace lib
{
    void trace(log::level prefix, std::uintptr_t fp, std::uintptr_t ip);
} // export namespace lib