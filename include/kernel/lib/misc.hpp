// Copyright (C) 2022-2023  ilobilo

#pragma once

// #include <init/kernel.hpp>
// #include <lib/math.hpp>
#include <type_traits>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <limits>
// #include <ranges>

// namespace detail
// {
//     template <typename C>
//     struct to_helper { };

//     template <typename Container, std::ranges::range Range>
//     requires std::convertible_to<std::ranges::range_value_t<Range>, typename Container::value_type>
//     Container operator|(Range &&r, to_helper<Container>)
//     {
//         return Container { r.begin(), r.end() };
//     }
// } // namespace detail

// template <std::ranges::range Container> requires (!std::ranges::view<Container>)
// auto to()
// {
//     return detail::to_helper<Container> { };
// }

constexpr inline bool remove_from(auto &container, auto &&val)
{
    return container.erase(std::remove(container.begin(), container.end(), val), container.end()) != container.end();
}

constexpr inline bool remove_from_if(auto &container, auto pred)
{
    return container.erase(std::remove_if(container.begin(), container.end(), pred), container.end()) != container.end();
}

// inline uint64_t seconds_since_boot(uint64_t seconds, uint64_t minutes, uint64_t hours, uint64_t days, uint64_t months, uint64_t years, uint64_t centuries)
// {
//     return epoch(seconds, minutes, hours, days, months, years, centuries) - boot_time_request.response->boot_time;
// }

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

// An experiment

// #include <mutex>

// template<typename Type>
// class mutex_wrapper
// {
//     private:
//     std::mutex lock;
//     Type storage;

//     public:
//     class locked_type
//     {
//         private:
//         std::unique_lock<std::mutex> locker;
//         Type &storage;

//         public:
//         constexpr locked_type(mutex_wrapper &parent) : locker(parent.lock), storage(parent.storage) { }
//         constexpr locked_type(mutex_wrapper &parent, std::adopt_lock_t al) : locker(parent.lock, al), storage(parent.storage) { }

//         constexpr locked_type(locked_type &&other) = default;
//         constexpr locked_type(const locked_type &other) = delete;

//         constexpr Type *operator->() noexcept { return std::addressof(this->storage); }
//         constexpr Type &operator*() noexcept { return this->storage; }
//         constexpr operator Type &() noexcept { return this->storage; }
//         constexpr Type &get() noexcept { return this->storage; }
//     };

//     template<typename ...Args>
//     constexpr explicit mutex_wrapper(Args &&...args) : storage(std::forward<Args>(args)...) { }

//     constexpr mutex_wrapper(const mutex_wrapper &other) noexcept = delete;
//     constexpr mutex_wrapper(mutex_wrapper &&other) noexcept = delete;

//     constexpr mutex_wrapper &operator=(const mutex_wrapper &other) noexcept = delete;
//     constexpr mutex_wrapper &operator=(mutex_wrapper &&other) noexcept = delete;

//     constexpr Type *operator->() noexcept { return std::addressof(this->storage); }
//     constexpr Type &operator*() noexcept { return this->storage; }
//     constexpr operator Type &() noexcept { return this->storage; }
//     constexpr Type &get() noexcept { return this->storage; }

//     [[nodiscard]] constexpr bool is_locked() { return this->lock.is_locked(); }
//     [[nodiscard]] constexpr locked_type with_lock() noexcept { return locked_type(*this); }
//     [[nodiscard]] constexpr std::optional<locked_type> try_with_lock() noexcept
//     {
//         if (this->lock.try_lock())
//             return locked_type(*this, std::adopt_lock);
//         return std::nullopt;
//     }
// };

// template<typename Type>
// mutex_wrapper(Type) -> mutex_wrapper<Type>;