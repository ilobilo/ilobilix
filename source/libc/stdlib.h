// Copyright (C) 2022  ilobilo

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void *oldptr, size_t size);
void free(void *ptr);

#ifdef __cplusplus
} // extern "C"
#endif