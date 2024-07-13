// Copyright (C) 2022-2024  ilobilo

#pragma once

#define va_start(v, l) __builtin_va_start(v, l)
#define va_arg(v, l) __builtin_va_arg(v, l)
#define va_copy(d, s) __builtin_va_copy(d, s)
#define va_end(v) __builtin_va_end(v)

typedef __builtin_va_list va_list;
