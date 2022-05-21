// Copyright (C) 2022  ilobilo

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

static inline int isalpha(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static inline int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

static inline int isxdigit(int c)
{
    return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static inline int isspace(char c)
{
    return c == ' ';
}

static inline int isupper(int c)
{
    return c >= 'A' && c <= 'Z';
}

static inline int islower(int c)
{
    return c >= 'a' && c <= 'z';
}

static inline char toupper(char c)
{
    return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

static inline char tolower(char c)
{
    return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

#ifdef __cplusplus
} // extern "C"
#endif