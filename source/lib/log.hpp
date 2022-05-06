// Copyright (C) 2022  ilobilo

#pragma once

namespace log
{
    static const char info_prefix[] = "[\033[32mINFO\033[0m] ";
    static const char warn_prefix[] = "[\033[33mWARN\033[0m] ";
    static const char error_prefix[] = "[\033[31mERROR\033[0m] ";

    int print(const char *fmt, ...);
    int println(const char *fmt = "", ...);
    int info(const char *fmt, ...);
    int warn(const char *fmt, ...);
    int error(const char *fmt, ...);
} // namespace log