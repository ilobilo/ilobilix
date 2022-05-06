// Copyright (C) 2022  ilobilo

#include <lib/string.hpp>
#include <limits.h>
#include <cstdint>
#include <cstddef>

size_t strlen(const char *str)
{
    if (str == nullptr) return 0;
    size_t length = 0;
    while(str[length]) length++;
    return length;
}

char *strcpy(char *destination, const char *source)
{
    if (destination == nullptr) return nullptr;
    char *ptr = destination;
    while (*source != '\0')
    {
        *destination = *source;
        destination++;
        source++;
    }
    *destination = '\0';
    return ptr;
}

char *strncpy(char *destination, const char *source, size_t n)
{
    if (destination == nullptr) return nullptr;
    char *ptr = destination;
    for (size_t i = 0; i < n && *source != '\0'; i++)
    {
        *destination = *source;
        destination++;
        source++;
    }
    *destination = '\0';
    return ptr;
}

int strcmp(const char *a, const char *b)
{
    if (a == nullptr || b == nullptr) return 1;
    while (*a && *b && *a == *b)
    {
        a++;
        b++;
    }
    return *a - *b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    if (a == nullptr || b == nullptr) return 1;
    for (size_t i = 0; i < n; i++)
    {
        if (a[i] != b[i]) return 1;
    }
    return 0;
}

void strrev(unsigned char *str)
{
    unsigned char a;
    unsigned len = strlen(reinterpret_cast<const char*>(str));

    for (size_t i = 0, j = len - 1; i < j; i++, j--)
    {
        a = str[i];
        str[i] = str[j];
        str[j] = a;
    }
}

long strtol(const char *nPtr, char **endPtr, int base)
{
    if((base < 2 || base > 36) && base != 0) return 0;

    long number = 0;
    const char * divider;
    int currentdigit, sign, cutlim;
    enum sign
    {
        NEGATIVE,
        POSITIVE
    };
    unsigned long cutoff;
    bool correctconversion = true;

    divider = nPtr;

    while (isspace(* divider)) divider++;

    if (* divider == '+')
    {
        sign = POSITIVE;
        divider++;
    }
    else if (* divider == '-')
    {
        sign = NEGATIVE;
        divider++;
    }
    else sign = POSITIVE;

    if (*divider == 0)
    {
        *endPtr = const_cast<char*>(divider);
        return 0;
    }

    if (*divider < '0' || (*divider > '9' && *divider < 'A') || (*divider > 'z')) return 0;

    if ((base == 8) && (*divider == '0'))
    {
        divider++;
        if (*divider == 'o' || *divider == 'O') divider++;
    }
    else if (base == 16)
    {
        if (*divider == '0')
        {
            divider++;
            if (*divider == 'x' || *divider == 'X')
            {
                divider++;
                if (*divider > 'f' || *divider > 'F')
                {
                    divider--;
                    *endPtr = const_cast<char*>(divider);
                    return 0;
                }
            }
            else divider--;
        }
    }
    else if (base == 0)
    {
        if (*divider == '0')
        {
            divider++;
            if (*divider == 'o' || *divider == 'O')
            {
                base = 8;
                divider++;
                if (*divider > '7')
                {
                    divider--;
                    *endPtr = const_cast<char*>(divider);
                    return 0;
                }
            }
            else if (*divider == 'x' || *divider == 'X')
            {
                base = 16;
                divider++;
                if (*divider > 'f' || * divider > 'F')
                {
                    divider--;
                    *endPtr = const_cast<char*>(divider);
                    return 0;
                }
            }
            else if (*divider <= '7') base = 8;
            else
            {
                *endPtr = const_cast<char*>(divider);
                return 0;
            }
        }
        else if (*divider >= '1' && *divider <= '9') base = 10;
    }

    if (sign) cutoff = LONG_MAX / static_cast<unsigned long>(base);
    else cutoff = static_cast<unsigned long>(LONG_MIN) / static_cast<unsigned long>(base);

    cutlim = cutoff % static_cast<unsigned long>(base);

    while (*divider != 0)
    {
        if (isdigit(*divider)) currentdigit = * divider - '0';
        else
        {
            if (isalpha(*divider))
            {
                if (islower(*divider) && (*divider - 'a') + 10 < base) currentdigit = (*divider - 'a') + 10;
                else if (!islower(*divider) && (*divider - 'A') + 10 < base) currentdigit = (*divider - 'A') + 10;
                else break;
            }
            else break;
        }
        if (!correctconversion || number > static_cast<long>(cutoff) || (number == static_cast<long>(cutoff) && static_cast<int>(currentdigit) > cutlim))
        {
            correctconversion = false;
            divider++;
        }
        else
        {
            correctconversion = true;
            number = (number * base) + currentdigit;
            divider++;
        }
    }
    if (!correctconversion)
    {
        if (sign) number = LONG_MAX;
        else number = LONG_MIN;
    }
    if (sign == NEGATIVE) number *= -1;
    if (endPtr != nullptr)
    {
        if (isspace(*divider)) divider++;
        *endPtr = const_cast<char*>(divider);
    }
    return number;
}

long long int strtoll(const char *nPtr, char **endPtr, int base)
{
    return strtol(nPtr, endPtr, base);
}
unsigned long strtoul(const char *nPtr, char **endPtr, int base)
{
    return strtol(nPtr, endPtr, base);
}
unsigned long long int strtoull(const char *nPtr, char **endPtr, int base)
{
    return strtol(nPtr, endPtr, base);
}

char *strstr(const char *str, const char *substr)
{
    const char *a = str, *b = substr;
    while (true)
    {
        if (!*b) return (char *)str;
        if (!*a) return nullptr;
        if (*a++ != *b++)
        {
            a = ++str;
            b = substr;
        }
    }
}

int itoa(int num, unsigned char *str, int len, int base)
{
    if (len == 0) return -1;

    int sum = num;
    int i = 0;
    int digit;

    do
    {
        digit = sum % base;
        if (digit < 0xA) str[i++] = '0' + digit;
        else str[i++] = 'A' + digit - 0xA;
        sum /= base;

    } while (sum && (1 < (len - 1)));

    if (i == (len - 1) && sum) return -1;
    str[i] = '\0';
    strrev(str);

    return 0;
}

int atoi(const char *str)
{
    return static_cast<int>(strtol(str, nullptr, 10));
}

int isspace(char c)
{
    if (c == ' ') return 1;
    return 0;
}

bool isempty(char *str)
{
    if (str == nullptr || strlen(str) == 0) return true;
    while (*str != '\0')
    {
        if (!isspace(*str)) return false;
        str++;
    }
    return true;
}

int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

int isalpha(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int islower(int c)
{
    return c >= 'a' && c <= 'z';
}

char tolower(char c)
{
    return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

char toupper(char c)
{
    return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

int tonum(char c)
{
    c = toupper(c);
    return (c) ? c - 64 : -1;
}