// Copyright (C) 2022  ilobilo

#include <drivers/smp.hpp>
#include <lib/lock.hpp>

// TODO: Separate errno per thread

extern "C" int *__errno_location()
{
    static irq_lock_t lock;
    lockit(lock);
    return reinterpret_cast<int*>(&this_cpu()->err);
}