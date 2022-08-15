// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

static constexpr uint64_t ATEXIT_MAX_FUNCS = 128;

extern "C"
{
    using uarch_t = unsigned;

    struct atexit_func_entry_t
    {
        void (*destructor_func)(void*);
        void *obj_ptr;
        void *dso_handle;
    };
} // extern "C"

namespace cxxabi
{
    void init();
} // namespace cxxabi