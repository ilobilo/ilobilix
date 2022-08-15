// Copyright (C) 2022  ilobilo

#pragma once

#include <cstddef>

struct [[gnu::packed]] driver_t
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

#define __DRIVER_depcount(deps...) sizeof((const char*[]){ deps }) / sizeof(char*)

#define __DRIVER_internal(_depcount, drv_name, deps_name, _name, _init, _fini, _deps...)  \
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

// Random with name
// #define __DRIVER_expand(name, init, fini, deps...) __DRIVER_internal(__DRIVER_depcount(deps), __DRIVER_glue(__driver_, name), __DRIVER_glue(__driver_deps_, name), #name, init, fini, deps)
// #define DRIVER(name, init, fini, deps...) __DRIVER_expand(name, init, fini, deps)

// Random with __COUNTER__ and __LINE__
#define __DRIVER_random __DRIVER_glue(__COUNTER__, __LINE__)
#define __DRIVER_expand(random, name, init, fini, deps...) __DRIVER_internal(__DRIVER_depcount(deps), __DRIVER_glue(__driver_, random), __DRIVER_glue(__driver_deps_, random), name, init, fini, deps)
#define DRIVER(name, init, fini, deps...) __DRIVER_expand(__DRIVER_random, name, init, fini, deps)