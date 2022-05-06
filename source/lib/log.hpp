// Copyright (C) 2022  ilobilo

#pragma once

namespace log
{
    int print(const char *fmt, ...);
    int info(const char *fmt, ...);
    int warn(const char *fmt, ...);
    int error(const char *fmt, ...);
} // namespace log