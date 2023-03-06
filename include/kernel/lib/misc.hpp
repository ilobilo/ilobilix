// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <init/kernel.hpp>
// #include <type_traits>
// #include <algorithm>
// #include <concepts>
// #include <utility>
// #include <cstdint>
// #include <cstddef>
// #include <memory>
// #include <cctype>
// #include <cerrno>
// #include <limits>
import std; // modules yay!

constexpr inline bool remove_from(auto &container, auto &val)
{
    return container.erase(std::remove(container.begin(), container.end(), val), container.end()) != container.end();
}

constexpr inline bool remove_from_if(auto &container, auto pred)
{
    return container.erase(std::remove_if(container.begin(), container.end(), pred), container.end()) != container.end();
}

constexpr inline auto align_down(auto n, auto a)
{
    return (n & ~(a - 1));
}

constexpr inline auto align_up(auto n, auto a)
{
    return align_down(n + a - 1, a);
}

constexpr inline auto div_roundup(auto n, auto a)
{
    return align_down(n + a - 1, a) / a;
}

constexpr inline auto next_pow2(uint64_t n)
{
    return n == 1UL ? 1UL : 1UL << (64 - __builtin_clzl(n - 1UL));
}

template<typename Type>
constexpr inline bool ishh(Type a)
{
    return uintptr_t(a) >= hhdm_offset;
}

template<typename Type>
constexpr inline Type tohh(Type a)
{
    return ishh(a) ? a : Type(uintptr_t(a) + hhdm_offset);
}

template<typename Type>
constexpr inline Type fromhh(Type a)
{
    return !ishh(a) ? a : Type(uintptr_t(a) - hhdm_offset);
}

template<typename Type>
constexpr inline Type pow(Type base, Type exp)
{
    int result = 1;
    for (; exp > 0; exp--)
        result *= base;
    return result;
}

template<typename Type>
constexpr inline Type abs(Type num)
{
    return num < 0 ? -num : num;
}

template<typename Type>
constexpr inline Type sign(Type num)
{
    return num > 0 ? 1 : (num < 0 ? -1 : 0);
}

constexpr inline uint64_t jdn(uint8_t days, uint8_t months, uint16_t years)
{
    return (1461 * (years + 4800 + (months - 14) / 12)) / 4 + (367 * (months - 2 - 12 * ((months - 14) / 12))) / 12 - (3 * ((years + 4900 + (months - 14) / 12) / 100)) / 4 + days - 32075;
}
constexpr inline uint64_t jdn_1970 = jdn(1, 1, 1970);

constexpr inline uint64_t epoch(uint64_t seconds, uint64_t minutes, uint64_t hours, uint64_t days, uint64_t months, uint64_t years, uint64_t centuries)
{
    uint64_t jdn_current = jdn(days, months, centuries * 100 + years);
    uint64_t diff = jdn_current - jdn_1970;

    return (diff * (60 * 60 * 24)) + hours * 3600 + minutes * 60 + seconds;
}

inline uint64_t seconds_since_boot(uint64_t seconds, uint64_t minutes, uint64_t hours, uint64_t days, uint64_t months, uint64_t years, uint64_t centuries)
{
    return epoch(seconds, minutes, hours, days, months, years, centuries) - boot_time_request.response->boot_time;
}

template<typename Ret>
constexpr inline Ret str2int(const char *nptr, char **endptr, int _base)
{
    using URet = std::make_unsigned_t<Ret>;

    auto base = static_cast<Ret>(_base);
    auto str = nptr;

    if (base < 0 || base == 1)
    {
        if (endptr != nullptr)
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

    if (endptr != nullptr)
        *endptr = const_cast<char*>(converted ? str : nptr);

    if (out_of_range)
    {
        errno = ERANGE;
        if constexpr (std::is_unsigned_v<Ret>)
            return std::numeric_limits<Ret>::max();
        else
            return negative ? std::numeric_limits<Ret>::min() : std::numeric_limits<Ret>::max();
    }

    return negative ? -total : total;
}

// template<typename Type>
// struct ref_val_wrapper
// {
//     private:
//     Type *_ref;
//     Type _val;

//     enum class which { /* other, */ ref, val };
//     which _which;

//     public:
//     constexpr ref_val_wrapper(Type &ref) : _ref(std::addressof(ref)), _which(which::ref) { }
//     constexpr ref_val_wrapper(Type &&val) : _val(std::move(val)), _which(which::val) { }

//     constexpr ref_val_wrapper(const ref_val_wrapper &other) : _which(other._which)
//     {
//         switch (this->_which)
//         {
//             case which::ref:
//                 this->_ref = other._ref;
//                 break;
//             case which::val:
//                 this->_val = other._val;
//                 break;
//             default:
//                 std::unreachable();
//         }
//     }

//     constexpr ref_val_wrapper(ref_val_wrapper &&other) : _which(std::move(other._which))
//     {
//         switch (this->_which)
//         {
//             case which::ref:
//                 this->_ref = std::move(other._ref);
//                 break;
//             case which::val:
//                 this->_val = std::move(other._val);
//                 break;
//             default:
//                 std::unreachable();
//         }
//     }

//     constexpr ref_val_wrapper &operator=(Type &ref)
//     {
//         this->_ref = std::addressof(ref);
//         this->_which = which::ref;
//         return *this;
//     }

//     constexpr ref_val_wrapper &operator=(Type &&val)
//     {
//         this->_val = std::move(val);
//         this->_which = which::val;
//         return *this;
//     }

//     constexpr ref_val_wrapper &operator=(ref_val_wrapper &other) = default;
//     constexpr ref_val_wrapper &operator=(ref_val_wrapper &&other) = default;

//     constexpr ref_val_wrapper &assign(Type &ref)
//     {
//         this->_val = ref;
//         this->_which = which::val;
//         return *this;
//     }

//     constexpr ref_val_wrapper &assign(Type &&val)
//     {
//         this->_val = std::move(val);
//         this->_which = which::val;
//         return *this;
//     }

//     constexpr Type &get()
//     {
//         switch (this->_which)
//         {
//             case which::ref:
//                 return *this->_ref;
//             case which::val:
//                 return this->_val;
//             default:
//                 std::unreachable();
//         }
//     }

//     constexpr bool is_ref()
//     {
//         switch (this->_which)
//         {
//             case which::ref:
//                 return true;
//             case which::val:
//                 return false;
//             default:
//                 std::unreachable();
//         }
//     }

//     operator Type() { return this->get(); }
// };

template<typename Type>
struct chain_wrapper
{
    private:
    chain_wrapper *_other;
    Type _val;
    bool _chained;

    public:
    constexpr chain_wrapper(Type val) : _val(val), _chained(false) { }

    constexpr chain_wrapper(const chain_wrapper &other) : _other(const_cast<chain_wrapper*>(std::addressof(other))), _chained(true) { }
    constexpr chain_wrapper(chain_wrapper &&other) = delete;

    constexpr chain_wrapper &operator=(Type val)
    {
        this->_val = val;
        this->_chained = false;
        return *this;
    }

    constexpr chain_wrapper &operator=(const chain_wrapper &other)
    {
        this->_other = const_cast<chain_wrapper*>(std::addressof(other));
        this->_chained = true;
        return *this;
    }

    constexpr Type &get()
    {
        if (this->_chained == true)
            return this->_other->get();
        return this->_val;
    }

    constexpr bool is_chained() const
    {
        return this->_chained;
    }

    operator Type() { return this->get(); }
};