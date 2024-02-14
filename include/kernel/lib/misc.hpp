// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <type_traits>
#include <memory>
#include <limits>

#include <cctype>
#include <cerrno>

template<typename Ret>
inline constexpr Ret str2int(const char *nptr, char **endptr, int _base)
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

    while (isspace(*str))
        str++;

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
        else
            break;

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


template<typename Type, typename Type1>
inline Type1 get_member_type(Type1 Type:: *);
#define MEMBER_TYPE(x) decltype(get_member_type(& x))