// Copyright (C) 2022  ilobilo

#pragma once

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef double max_align_t;

#undef NULL

#ifdef __cplusplus
# define NULL __null
#else
# define NULL ((void*)0)
#endif

#define offsetof(type, member) __builtin_offsetof(type, member)