// Copyright (C) 2021-2022  ilobilo

#include <cpu/smp/smp.hpp>

// TODO: errno per thread
void errno_set(errno_t err)
{
    this_cpu->err = err;
}

errno_t errno_get()
{
    return this_cpu->err;
}