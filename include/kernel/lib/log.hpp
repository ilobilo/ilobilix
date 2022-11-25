// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <format>

namespace log
{
    static constexpr auto info_prefix = "[\033[32mINFO\033[0m] ";
    static constexpr auto warn_prefix = "[\033[33mWARN\033[0m] ";
    static constexpr auto error_prefix = "[\033[31mERROR\033[0m] ";

    inline lock_t lock;

    void prints(const char *str, size_t length);
    void prints(const char *str);
    void printc(char c);

    namespace detail
    {
        inline auto vprint(std::string_view fmt, fmt::format_args args) -> size_t
        {
            struct
            {
                using value_type = char;
                void push_back(char val)
                {
                    printc(val);
                    this->_length++;
                }
                size_t _length;
            } printer;

            fmt::vformat_to(std::back_inserter(printer), fmt, args);
            return printer._length;
        }

        template<typename ...Args> requires (sizeof...(Args) > 0)
        inline auto print(std::string_view fmt, Args &&...args) -> size_t
        {
            return vprint(fmt, fmt::make_format_args(args...));
        }

        template<typename ...Args>
        inline auto print(std::string_view fmt, Args &&...) -> size_t
        {
            prints(fmt.data(), fmt.length());
            return fmt.length();
        }
    } // namespace detail

    template<typename ...Args>
    inline auto print(std::string_view fmt, Args &&...args) -> size_t
    {
        lockit(lock);
        return detail::print(fmt, args...);
    }

    template<typename ...Args>
    inline auto println(std::string_view fmt = "", Args &&...args) -> size_t
    {
        lockit(lock);
        auto ret = detail::print(fmt, args...);
        printc('\n');
        return ret + 1;
    }

    #define PRINT_FUNC(name)                                                                   \
        template<typename ...Args>                                                             \
        inline auto name(std::string_view fmt = "", Args &&...args) -> size_t                  \
        {                                                                                      \
            lockit(lock);                                                                      \
            prints(name ## _prefix);                                                           \
            return detail::print(fmt, args...);                                                \
        }                                                                                      \
        template<typename ...Args>                                                             \
        inline auto name ## ln(std::string_view fmt = "", Args &&...args) -> size_t            \
        {                                                                                      \
            lockit(lock);                                                                      \
            prints(name ## _prefix);                                                           \
            auto ret = detail::print(fmt, args...);                                            \
            printc('\n');                                                                      \
            return ret + 1;                                                                    \
        }

    PRINT_FUNC(info)
    PRINT_FUNC(warn)
    PRINT_FUNC(error)

    #undef PRINT_FUNC
} // namespace log