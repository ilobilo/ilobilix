// Copyright (C) 2022  ilobilo

#pragma once

#include <init/kernel.hpp>
#include <type_traits>
#include <concepts>
#include <cstdint>
#include <cctype>
#include <limits>

struct point
{
    size_t X = 0;
    size_t Y = 0;
};

constexpr uint64_t align_down(uint64_t n, uint64_t a)
{
    return (n & ~(a - 1));
}

constexpr uint64_t align_up(uint64_t n, uint64_t a)
{
    return align_down(n + a - 1, a);
}

constexpr uint64_t div_roundup(uint64_t n, uint64_t a)
{
    return align_down(n + a - 1, a) / a;
}

constexpr uint64_t next_pow2(uint64_t n)
{
    return n == 1 ? 1 : 1 << (64 - __builtin_clzl(n - 1));
}

template<typename Enum>
constexpr auto as_int(Enum const value) -> typename std::underlying_type<Enum>::type
{
    return static_cast<typename std::underlying_type<Enum>::type>(value);
}

template<std::integral Type>
constexpr bool ishh(Type a)
{
    return static_cast<uint64_t>(a) >= hhdm_offset;
}

template<std::integral Type>
constexpr Type tohh(Type a)
{
    return ishh(a) ? a : static_cast<Type>(static_cast<uint64_t>(a) + hhdm_offset);
}

template<std::integral Type>
constexpr Type fromhh(Type a)
{
    return !ishh(a) ? a : static_cast<Type>(static_cast<uint64_t>(a) - hhdm_offset);
}

template<typename Type>
constexpr bool ishh(Type a)
{
    return reinterpret_cast<uint64_t>(a) >= hhdm_offset;
}

template<typename Type>
constexpr Type tohh(Type a)
{
    return ishh(a) ? a : reinterpret_cast<Type>(reinterpret_cast<uint64_t>(a) + hhdm_offset);
}

template<typename Type>
constexpr Type fromhh(Type a)
{
    return !ishh(a) ? a : reinterpret_cast<Type>(reinterpret_cast<uint64_t>(a) - hhdm_offset);
}

template<typename Type>
constexpr Type pow(Type base, Type exp)
{
    int result = 1;
    for (; exp > 0; exp--)
        result *= base;
    return result;
}

template<typename Type>
constexpr Type abs(Type num)
{
    return num < 0 ? -num : num;
}

template<typename Type>
constexpr Type sign(Type num)
{
    return (num > 0) ? 1 : ((num < 0) ? -1 : 0);
}

constexpr uint64_t jdn(uint8_t days, uint8_t months, uint16_t years)
{
    return (1461 * (years + 4800 + (months - 14) / 12)) / 4 + (367 * (months - 2 - 12 * ((months - 14) / 12))) / 12 - (3 * ((years + 4900 + (months - 14) / 12) / 100)) / 4 + days - 32075;
}
constexpr uint64_t jdn_1970 = jdn(1, 1, 1970);

constexpr uint64_t epoch(uint64_t seconds, uint64_t minutes, uint64_t hours, uint64_t days, uint64_t months, uint64_t years, uint64_t centuries)
{
    uint64_t jdn_current = jdn(days, months, centuries * 100 + years);
    uint64_t diff = jdn_current - jdn_1970;

    return (diff * (60 * 60 * 24)) + hours * 3600 + minutes * 60 + seconds;
}

inline uint64_t seconds_since_boot(uint64_t seconds, uint64_t minutes, uint64_t hours, uint64_t days, uint64_t months, uint64_t years, uint64_t centuries)
{
    return epoch(seconds, minutes, hours, days, months, years, centuries) - boot_time_request.response->boot_time;
}

extern "C" int *__errno_location();
template<typename Ret, typename URet = std::make_unsigned_t<Ret>>
static Ret str2int(const char *nptr, char **endptr, int _base)
{
    auto base = static_cast<Ret>(_base);
    auto str = nptr;

    if (base < 0 || base == 1)
    {
        if (endptr)
            *endptr = const_cast<char*>(nptr);
        return 0;
    }

    while (isspace(*str)) str++;

    bool negative = false;
    if (*str == '-')
    {
        negative = true;
        str++;
    }
    else if (*str == '+')
        str++;

    bool octal = (str[0] == '0');
    bool hex = octal && (str[1] == 'x' || str[1] == 'X');

    if ((base == 0 || base == 16) && hex == true && isxdigit(str[2]))
    {
        str += 2;
        base = 16;
    }
    else if ((base == 0 || base == 8) && octal == true)
        base = 8;
    else if (base == 0)
        base = 10;

    URet cutoff = 0;
    URet cutlim = 0;
    if constexpr (std::is_unsigned_v<Ret>)
    {
        cutoff = std::numeric_limits<Ret>::max() / base;
        cutoff = std::numeric_limits<Ret>::max() % base;
    }
    else
    {
        Ret co = negative ? std::numeric_limits<Ret>::min() : std::numeric_limits<Ret>::max();
        cutlim = negative ? -(co % base) : co % base;
        co /= negative ? -base : base;
        cutoff = co;
    }

    URet total = 0;
    bool converted = false;
    bool out_of_range = false;

    for (char c = *str; c != '\0'; c = *++str)
    {
        URet digit = 0;
        if (isdigit(c))
            digit = c - '0';
        else if (isupper(c))
            digit = c - 'A' + 0x10;
        else if (islower(c))
            digit = c - 'a' + 0x10;
        else break;

        if (digit >= static_cast<URet>(base))
            break;

        if (out_of_range)
            continue;

        if (total >= cutoff || (total == cutoff && digit > cutlim))
            out_of_range = true;
        else
        {
            total = (total * base) + digit;
            converted = true;
        }
    }

    if (endptr)
        *endptr = const_cast<char*>(converted ? str : nptr);

    if (out_of_range)
    {
        *__errno_location() = 34;
        if constexpr (std::is_unsigned_v<Ret>)
            return std::numeric_limits<Ret>::max();
        else
            return negative ? std::numeric_limits<Ret>::min() : std::numeric_limits<Ret>::max();
    }

    return negative ? -total : total;
}

// https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector

#define GENERATE_HAS_MEMBER_TYPE(type)                                                                            \
    template<typename Type>                                                                                       \
    class _HasMemberType_##type                                                                                   \
    {                                                                                                             \
        private:                                                                                                  \
        using Yes = char[2];                                                                                      \
        using No = char[1];                                                                                       \
                                                                                                                  \
        struct Fallback                                                                                           \
        {                                                                                                         \
            struct type { };                                                                                      \
        };                                                                                                        \
        struct Derived : Type, Fallback { };                                                                      \
                                                                                                                  \
        template<typename U>                                                                                      \
        static No &test(typename U::type *);                                                                      \
                                                                                                                  \
        template<typename U>                                                                                      \
        static Yes &test(U *);                                                                                    \
                                                                                                                  \
        public:                                                                                                   \
        static constexpr bool RESULT = sizeof(test<Derived>(nullptr)) == sizeof(Yes);                             \
    };                                                                                                            \
                                                                                                                  \
    template<typename Type>                                                                                       \
    struct has_member_type_##type : public std::integral_constant<bool, _HasMemberType_##type<Type>::RESULT> { };