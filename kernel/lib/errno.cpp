// Copyright (C) 2022  ilobilo

#include <drivers/proc.hpp>
#include <drivers/smp.hpp>

extern "C" int *__errno_location()
{
    auto thread = this_thread();
    return thread ? reinterpret_cast<int*>(&thread->error) : reinterpret_cast<int*>(&this_cpu()->error);
}