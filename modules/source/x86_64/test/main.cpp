// Copyright (C) 2022  ilobilo

#include <modules/module.hpp>
#include <cstdio>

bool test_init()
{
    printf("Hello from x86_64 test driver!\n");
    return true;
}

bool test_fini()
{
    printf("Goodbye from x86_64 test driver!\n");
    return true;
}

DRIVER("test1", test_init, test_fini);