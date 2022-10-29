// Copyright (C) 2022  ilobilo

#include <cstdlib>
#include <cstddef>
#include <cstdint>

extern "C"
{
    [[gnu::weak]] void *memcpy(void *dest, const void *src, size_t len)
    {
        uint8_t *pdest = static_cast<uint8_t*>(dest);
        const uint8_t *psrc = static_cast<const uint8_t*>(src);

        for (size_t i = 0; i < len; i++)
            pdest[i] = psrc[i];

        return dest;
    }

    [[gnu::weak]] void *memset(void *dest, int ch, size_t len)
    {
        uint8_t *p = static_cast<uint8_t*>(dest);

        for (size_t i = 0; i < len; i++)
            p[i] = static_cast<uint8_t>(ch);

        return dest;
    }

    [[gnu::weak]] void *memmove(void *dest, const void *src, size_t len)
    {
        uint8_t *pdest = static_cast<uint8_t*>(dest);
        const uint8_t *psrc = static_cast<const uint8_t*>(src);

        if (src > dest)
            for (size_t i = 0; i < len; i++)
                pdest[i] = psrc[i];
        else if (src < dest)
            for (size_t i = len; i > 0; i--)
                pdest[i - 1] = psrc[i - 1];

        return dest;
    }

    [[gnu::weak]] int memcmp(const void *ptr1, const void *ptr2, size_t len)
    {
        const uint8_t *p1 = static_cast<const uint8_t*>(ptr1);
        const uint8_t *p2 = static_cast<const uint8_t*>(ptr2);

        for (size_t i = 0; i < len; i++)
            if (p1[i] != p2[i])
                return p1[i] < p2[i] ? -1 : 1;

        return 0;
    }

    [[gnu::weak]] void *memchr(const void *ptr, int ch, size_t len)
    {
        const uint8_t *src = static_cast<const uint8_t*>(ptr);

        while (len-- > 0)
        {
            if (*src == ch)
                return const_cast<uint8_t*>(src);
            src++;
        }

        return nullptr;
    }

    size_t strlen(const char *str)
    {
        size_t length = 0;
        while (str[length])
            length++;
        return length;
    }

    char *strdup(const char *str)
    {
        size_t len = strlen(str) + 1;

        void *newstr = malloc(len);
        if (newstr == nullptr)
            return nullptr;

        return static_cast<char*>(memcpy(newstr, str, len));
    }

    char *strcat(char *dest, const char *src)
    {
        char *ptr = dest + strlen(dest);
        while (*src != '\0') *ptr++ = *src++;
        *ptr = '\0';
        return dest;
    }

    char *strncat(char *dest, const char *src, size_t len)
    {
        char* ptr = dest + strlen(dest);
        while (*src != '\0' && len--)
            *ptr++ = *src++;

        *ptr = '\0';
        return dest;
    }

    char *strchr(const char *str, int ch)
    {
        while (*str && *str != ch)
            str++;
        return const_cast<char*>(ch == *str ? str : nullptr);
    }

    int strcmp(const char *str1, const char *str2)
    {
        while (*str1 && *str2 && *str1 == *str2)
        {
            str1++;
            str2++;
        }
        return *str1 - *str2;
    }

    int strncmp(const char *str1, const char *str2, size_t len)
    {
        while (*str1 && *str2 && *str1 == *str2 && len--)
        {
            str1++;
            str2++;
        }
        if (len == 0)
            return 0;
        return *str1 - *str2;
    }

    char *strcpy(char *dest, const char *src)
    {
        char *ptr = dest;
        while (*src != '\0')
        {
            *dest = *src;
            dest++;
            src++;
        }
        *dest = '\0';
        return ptr;
    }

    char *strncpy(char *dest, const char *src, size_t len)
    {
        char *ptr = dest;
        while (*src != '\0' && len--)
        {
            *dest = *src;
            dest++;
            src++;
        }
        *dest = '\0';
        return ptr;
    }

    char *strstr(const char *str, const char *substr)
    {
        const char *a = str, *b = substr;
        while (true)
        {
            if (*b == 0)
                return const_cast<char*>(str);
            if (*a == 0)
                return nullptr;

            if (*a++ != *b++)
            {
                a = ++str;
                b = substr;
            }
        }
    }

    void strrev(char *str)
    {
        char a;
        size_t len = strlen(reinterpret_cast<const char*>(str));

        for (size_t i = 0, j = len - 1; i < j; i++, j--)
        {
            a = str[i];
            str[i] = str[j];
            str[j] = a;
        }
    }
} // extern "C"