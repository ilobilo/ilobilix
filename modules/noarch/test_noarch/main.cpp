// Copyright (C) 2022-2023  ilobilo

#include <lib/log.hpp>
#include <module.hpp>

#include <fmt/core.h>
#include <stdio.h>

// Always put DRIVER() before init and fini functions
// __init__ and __fini__ are currently just extern "C"
// test3 depends on test1. multiple dependencies are supported
DRIVER(test3, init, fini, "test1")

__init__ bool init()
{
    log::println("Hello from first noarch test driver! {}", "(this should run after test1 driver)");
    return true;
}

__fini__ bool fini()
{
    log::println("{}", fmt::format("Goodbye from first {} test driver!\n", "noarch"));
    return true;
}

DRIVER(test4, init4, fini4, "test3")

__init__ bool init4()
{
    printf("Hello from second noarch test driver! %s\n", "(this should run after test3 driver)");
    return true;
}

__fini__ bool fini4()
{
    log::println("Goodbye from second {} test driver!\n", "noarch");
    return true;
}