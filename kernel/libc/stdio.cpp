// Copyright (C) 2022  ilobilo

#include <printf/printf.h>
#include <lib/log.hpp>
#include <cstdlib>
#include <cstdio>

extern "C"
{
    static lock_t lock;

    int fputc(char c, FILE *stream)
    {
        log::printc(c);
        return c;
        // return log::print("{}", c);
    }

    int fputs(const char *str, FILE *stream)
    {
        return log::print("{}{}", (stream == stderr) ? log::error_prefix : "", str);
    }

    int fputws(const wchar_t *str, FILE *stream) { return -1; }

    int fprintf(FILE *stream, const char *format, ...)
    {
        lockit(lock);

        va_list arg;
        va_start(arg, format);

        if (stream == stderr)
            log::prints(log::error_prefix);

        int ret = vprintf(format, arg);

        va_end(arg);
        return ret;
    }

    size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
    {
        lockit(lock);

        if (stream == stderr)
            log::prints(log::error_prefix);

        const uint8_t *uptr = static_cast<const uint8_t*>(ptr);
        for (size_t i = 0; i < size * nmemb; i += size)
            log::prints(reinterpret_cast<const char *>(uptr + i), size);

        return nmemb;
    }

    int vprintf(const char *format, va_list arg)
    {
        lockit(lock);
        return vprintf_(format, arg);
    }

    int printf(const char *format, ...)
    {
        va_list arg;
        va_start(arg, format);

        int ret = vprintf(format, arg);

        va_end(arg);
        return ret;
    }

    int vsprintf(char *str, const char *format, va_list arg)
    {
        return vsprintf_(str, format, arg);
    }

    int sprintf(char *str, const char *format, ...)
    {
        va_list arg;
        va_start(arg, format);

        int ret = vsprintf(str, format, arg);

        va_end(arg);
        return ret;
    }

    int vsnprintf(char *str, size_t count, const char *format, va_list arg)
    {
        return vsnprintf_(str, count, format, arg);
    }

    int snprintf(char *str, size_t count, const char *format, ...)
    {
        va_list arg;
        va_start(arg, format);

        int ret = vsnprintf(str, count, format, arg);

        va_end(arg);
        return ret;
    }

    int vasprintf(char **str, const char *format, va_list arg)
    {
        va_list tmpa;
        va_copy(tmpa, arg);

        int ret = vsnprintf(NULL, 0, format, tmpa);

        va_end(tmpa);
        if (ret < 0)
            return -1;

        *str = malloc<char*>(ret + 1);
        if (*str == nullptr)
            return -1;

        ret = vsprintf(*str, format, arg);
        return ret;
    }

    int asprintf(char **str, const char *format, ...)
    {
        va_list arg;
        va_start(arg, format);

        int ret = vasprintf(str, format, arg);

        va_end(arg);
        return ret;
    }
} // extern "C"