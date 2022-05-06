// Copyright (C) 2022  ilobilo

#pragma once

#include <cstddef>

unsigned constexpr hash(const char *input)
{
    return *input ? static_cast<unsigned int>(*input) + 33 * hash(input + 1) : 5381;
}

size_t strlen(const char *str);

char *strcpy(char *destination, const char *source);
char *strncpy(char *destination, const char *source, size_t n);

int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);

void strrev(unsigned char *str);

long strtol(const char *nPtr, char **endPtr, int base);
long long int strtoll(const char *nPtr, char **endPtr, int base);
unsigned long strtoul(const char *nPtr, char **endPtr, int base);
unsigned long long int strtoull(const char *nPtr, char **endPtr, int base);

char *strstr(const char *str, const char *substr);

int isspace(char c);

int isdigit(int c);
int isalpha(int c);
int islower(int c);

char tolower(char c);
char toupper(char c);

int itoa(int num, unsigned char *str, int len, int base);
int atoi(const char *str);

bool isempty(char *str);
static inline bool isempty(const char *str)
{
    return isempty(const_cast<char*>(str));
}

int tonum(char c);

extern "C" void *memcpy(void *dest, const void *src, size_t len);
extern "C" int memcmp(const void *ptr1, const void *ptr2, size_t len);
extern "C" void *memset(void *dest, int ch, size_t n);
extern "C" void *memmove(void *dest, const void *src, size_t len);