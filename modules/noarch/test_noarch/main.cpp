// Copyright (C) 2022  ilobilo

#include <modules/module.hpp>
#include <lib/log.hpp>

#include <fmt/printf.h>

// Always put DRIVER() before init and fini functions
// __init and __fini are currently just extern "C"
// test1 depends on test2. multiple dependencies are supported
DRIVER(test3, init, fini, "test1")

__init bool init()
{
    log::println("Hello from first noarch test driver! {}", "(this should run after test1 driver)");
    return true;
}

__fini bool fini()
{
    fmt::print("Goodbye from first {} test driver!\n", "noarch");
    return true;
}

DRIVER(test4, init4, fini4, "test3")

__init bool init4()
{
    printf("Hello from second noarch test driver! %s\n", "(this should run after test3 driver)");
    return true;
}

__fini bool fini4()
{
    fmt::printf("Goodbye from second %s test driver!\n", "noarch");
    return true;
}