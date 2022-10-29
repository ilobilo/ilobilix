// Copyright (C) 2022  ilobilo

#pragma once

#include <frg/formatting.hpp>
#include <string>

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

    void toggle_term(bool on);
    bool to_term();

    template<typename ...Args>
    inline auto print(std::string_view fmt, Args &&...args) -> size_t
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

    template<typename ...Args>
    inline auto println(std::string_view fmt = "", Args &&...args) -> size_t
    {
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
            return println(fmt, args...);                            \
        }

    PRINT_FUNC(info)
    PRINT_FUNC(warn)
    PRINT_FUNC(error)

    #undef PRINT_FUNC

    template<typename ...Args>
    inline auto fmt(std::string_view fmt, Args &&...args) -> std::string
    {
        struct
        {
            void append(char val)
            {
                this->_buffer += val;
            }

            void append(const char *val)
            {
                this->_buffer += val;
            }

            std::string _buffer;
        } formatter;

        frg::format(frg::fmt({ fmt.data(), fmt.length() }, args...), formatter);
        return formatter._buffer;
    }
} // namespace log