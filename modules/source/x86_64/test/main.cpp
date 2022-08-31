// Copyright (C) 2022  ilobilo

#include <modules/module.hpp>
#include <cstdio>

// Always put DRIVER() until init and fini functions
DRIVER(test, init, fini)

__init bool init()
{
    printf("Hello from x86_64 test driver!\n");
    return true;
}

__fini bool fini()
{
    printf("Goodbye from x86_64 test driver!\n");
    return true;
}