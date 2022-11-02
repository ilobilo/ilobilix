// Copyright (C) 2022  ilobilo

#pragma once

#include <format>

namespace frg
{
    template<typename F>
    void format_object(const std::string_view &object, format_options, F &formatter)
    {
        for (const auto c : object)
            formatter.append(c);
    }

    template<typename F>
    void format_object(const std::string &object, format_options, F &formatter)
    {
        formatter.append(object.c_str());
    }
} // namespace frg

namespace log
{
    static constexpr auto info_prefix = "[\033[32mINFO\033[0m] ";
    static constexpr auto warn_prefix = "[\033[33mWARN\033[0m] ";
    static constexpr auto error_prefix = "[\033[31mERROR\033[0m] ";

    static lock_t lock;

    void prints(const char *str, size_t length);
    void prints(const char *str);
    void printc(char c);

    template<typename ...Args> requires (sizeof...(Args) > 0)
    inline auto print(std::string_view fmt, Args &&...args) -> size_t
    {
        if (heap::allocator.valid() == false)
        {
            struct
            {
                void append(char val)
                {
                    printc(val);
                    this->_length++;
                }

                void append(const char *val)
                {
                    prints(val);
                    this->_length += strlen(val);
                }

                size_t _length;
            } printer;

            frg::format(frg::fmt({ fmt.data(), fmt.length() }, args...), printer);
            return printer._length;
        }

        auto str = fmt::vformat(fmt, fmt::make_format_args(args...));
        prints(str.c_str());
        return str.length();
    }

    template<typename ...Args>
    inline auto print(std::string_view fmt, Args &&...args) -> size_t
    {
        prints(fmt.data(), fmt.length());
        return fmt.length();
    }

    template<typename ...Args>
    inline auto println(std::string_view fmt = "", Args &&...args) -> size_t
    {
        lockit(lock);
        auto ret = print(fmt, args...);
        printc('\n');
        return ret + 1;
    }

    #define PRINT_FUNC(name)                                         \
        template<typename ...Args>                                   \
        inline auto name(std::string_view fmt, Args &&...args)       \
        {                                                            \
            lockit(lock);                                            \
            prints(name ## _prefix);                                 \
            return print(fmt, args...);                              \
        }                                                            \
        template<typename ...Args>                                   \
        inline auto name ## ln(std::string_view fmt, Args &&...args) \
        {                                                            \
            lockit(lock);                                            \
            prints(name ## _prefix);                                 \
            auto ret = print(fmt, args...);                          \
            printc('\n');                                            \
            return ret + 1;                                          \
        }

    PRINT_FUNC(info)
    PRINT_FUNC(warn)
    PRINT_FUNC(error)

    #undef PRINT_FUNC
} // namespace log