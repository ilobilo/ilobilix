// Copyright (C) 2022  ilobilo

#pragma once

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char *format, ...);
int vprintf(const char *format, va_list arg);
int sprintf(char *str, const char *format, ...);
int vsprintf(char *str, const char *format, va_list arg);
int snprintf(char *str, size_t count, const char *format, ...);
int vsnprintf(char *str, size_t count, const char *format, va_list arg);
int vasprintf(char **str, const char *format, va_list args);
int asprintf(char **str, const char *format, ...);

#ifdef __cplusplus
} // extern "C"
#endif