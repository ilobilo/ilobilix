// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <cstddef>

struct [[gnu::packed, gnu::aligned(8)]] driver_t
{
    const char *name;
    bool initialised;
    bool (*init)();
    bool (*fini)();
    size_t depcount;
    const char **deps;
};

#define DRIVER_SECTION ".drivers"
#define DRIVER_DEPS_SECTION ".drivers.deps"

#define __DRIVER_glue_impl(x, y) x ## y
#define __DRIVER_glue(x, y) __DRIVER_glue_impl(x, y)

#define __DRIVER_pragma(x) _Pragma(#x)

#define __DRIVER_rename_get(random, func) __DRIVER_glue(func, random)
#define __DRIVER_rename_impl(old_name, new_name)        \
    __DRIVER_pragma(redefine_extname old_name new_name) \
    extern "C" bool new_name();

#define __DRIVER_rename(random, func) __DRIVER_rename_impl(func, __DRIVER_rename_get(random, func))

#define __DRIVER_random(name) __DRIVER_glue(_, __DRIVER_glue(name, __DRIVER_glue(__COUNTER__, __LINE__)))
#define __DRIVER_depcount(deps...) sizeof((const char*[]){ deps }) / sizeof(char*)

#define __DRIVER_internal(_depcount, drv_name, deps_name, _init, _fini, _name, _deps...)  \
    extern "C" [[gnu::section(DRIVER_DEPS_SECTION), gnu::used]]                           \
    const char *deps_name[_depcount] = { _deps };                                         \
                                                                                          \
    extern "C" [[gnu::section(DRIVER_SECTION), gnu::used]]                                \
    const driver_t drv_name =                                                             \
    {                                                                                     \
        .name = _name,                                                                    \
        .initialised = false,                                                             \
        .init = _init,                                                                    \
        .fini = _fini,                                                                    \
        .depcount = _depcount,                                                            \
        .deps = deps_name                                                                 \
    };

#define __DRIVER_expand(random, name, init, fini, deps...) \
    __DRIVER_rename(random, init)                          \
    __DRIVER_rename(random, fini)                          \
    __DRIVER_internal(                                     \
        __DRIVER_depcount(deps),                           \
        __DRIVER_glue(__driver, random),                   \
        __DRIVER_glue(__driver_deps, random),              \
        __DRIVER_rename_get(random, init),                 \
        __DRIVER_rename_get(random, fini),                 \
        #name, deps                                        \
    )

#define DRIVER(name, init, fini, deps...) __DRIVER_expand(__DRIVER_random(name), name, init, fini, deps)

#define __init__ extern "C"
#define __fini__ extern "C"