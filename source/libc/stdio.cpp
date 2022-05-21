// Copyright (C) 2022  ilobilo

#define PRINTF_ALIAS_STANDARD_FUNCTION_NAMES 1
#include <printf/printf.h>
#include <lib/alloc.hpp>

extern "C"
{
    int vasprintf(char **str, const char *format, va_list args)
    {
        va_list tmpa;
        va_copy(tmpa, args);

        int ret = vsnprintf(NULL, 0, format, tmpa);

        va_end(tmpa);
        if (ret < 0) return -1;

        *str = malloc<char*>(ret + 1);
        if (*str == nullptr) return -1;

        ret = vsprintf(*str, format, args);
        return ret;
    }

    int asprintf(char **str, const char *format, ...)
    {
        va_list args;
        va_start(args, format);

        int ret = vasprintf(str, format, args);

        va_end(args);
        return ret;
    }
} // extern "C"