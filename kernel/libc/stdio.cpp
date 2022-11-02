// Copyright (C) 2022  ilobilo

#include <lib/log.hpp>
#include <cstdlib>
#include <cstdio>

extern "C"
{
    int fputc(char c, FILE *stream)
    {
        log::printc(c);
        return c;
    }

    int fputs(const char *str, FILE *stream)
    {
        if (stream == stderr)
            log::prints(log::error_prefix);

        log::prints(str);
        return 0;
    }

    int fprintf(FILE *stream, const char *format, ...)
    {
        va_list args;
        va_start(args, format);

        if (stream == stderr)
            log::prints(log::error_prefix);

        int ret = vprintf(format, args);

        va_end(args);
        return ret;
    }

    size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
    {
        if (stream == stderr)
            log::prints(log::error_prefix);

        const uint8_t *uptr = static_cast<const uint8_t*>(ptr);
        for (size_t i = 0; i < size * nmemb; i += size)
            log::prints(reinterpret_cast<const char *>(uptr + i), size);

        return nmemb;
    }

    int vasprintf(char **str, const char *format, va_list args)
    {
        va_list tmpa;
        va_copy(tmpa, args);

        int ret = vsnprintf(NULL, 0, format, tmpa);

        va_end(tmpa);
        if (ret < 0)
            return -1;

        *str = malloc<char*>(ret + 1);
        if (*str == nullptr)
            return -1;

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