// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <lib/types.hpp>

namespace time
{
    int sys_nanosleep(timespec *req, timespec *rem);
    int sys_clock_gettime(clockid_t clockid, timespec *tp);
} // namespace time