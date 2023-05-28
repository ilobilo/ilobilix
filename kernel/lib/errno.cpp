// Copyright (C) 2022-2023  ilobilo

#include <drivers/proc.hpp>
#include <drivers/smp.hpp>

extern "C" errno_t *__errno_location()
{
    auto thread = this_thread();
    return reinterpret_cast<errno_t*>(thread ? &thread->error : &this_cpu()->error);
}