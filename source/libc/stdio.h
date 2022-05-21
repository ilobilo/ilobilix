// Copyright (C) 2022  ilobilo

#pragma once

#define PRINTF_ALIAS_STANDARD_FUNCTION_NAMES 1
#include <printf/printf.h>

#ifdef __cplusplus
extern "C" {
#endif

int vasprintf(char **str, const char *format, va_list args);
int asprintf(char **str, const char *format, ...);

#ifdef __cplusplus
} // extern "C"
#endif