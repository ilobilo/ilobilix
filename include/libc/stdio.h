// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <stddef.h>
#include <stdarg.h>

#define ATTR_FORMAT(fmt_index, first_arg) __attribute__((format(printf, (fmt_index), (first_arg))))
#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

// Stubs for fmtlib
typedef size_t FILE;
extern FILE *stdout;
extern FILE *stderr;

int fflush (FILE *stream);

void flockfile(FILE *stream);
void funlockfile(FILE *stream);

int fputc(char c, FILE *stream);
int fputs(const char *str, FILE *stream);
int fputws(const wchar_t *str, FILE *stream);

int fprintf(FILE *stream, const char *format, ...);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

int printf(const char *format, ...) ATTR_FORMAT(1, 2);
int vprintf(const char *format, va_list arg) ATTR_FORMAT(1, 0);

int sprintf(char *str, const char *format, ...) ATTR_FORMAT(2, 3);
int vsprintf(char *str, const char *format, va_list arg) ATTR_FORMAT(2, 0);

int snprintf(char *str, size_t count, const char *format, ...) ATTR_FORMAT(3, 4);
int vsnprintf(char *str, size_t count, const char *format, va_list arg) ATTR_FORMAT(3, 0);

int vasprintf(char **str, const char *format, va_list arg) ATTR_FORMAT(2, 0);
int asprintf(char **str, const char *format, ...) ATTR_FORMAT(2, 3);

int fctprintf(void (*out)(char c, void *extra_arg), void *extra_arg, const char *format, ...);
int vfctprintf(void (*out)(char c, void *extra_arg), void *extra_arg, const char *format, va_list arg);

#ifdef __cplusplus
} // extern "C"

namespace std
{
    using ::FILE;

    using ::fflush;

    using ::flockfile;
    using ::funlockfile;

    using ::fputc;
    using ::fputs;
    using ::fputws;

    using ::fprintf;
    using ::fwrite;

    using ::printf;
    using ::vprintf;

    using ::sprintf;
    using ::vsprintf;

    using ::snprintf;
    using ::vsnprintf;

    using ::vasprintf;
    using ::asprintf;

    using ::fctprintf;
    using ::vfctprintf;

    [[gnu::noreturn]] void terminate() noexcept;
} // namespace std

#endif
