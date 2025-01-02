// Copyright (C) 2024  ilobilo

export module lib:log;

import :math;
import std;

namespace log
{
    std::mutex _lock;
    std::uint64_t get_time();

    export namespace unsafe
    {
        void prints(std::string_view str);
        void printc(char chr);

        inline void lock() { _lock.lock(); }
        inline void unlock() { _lock.unlock(); }
    } // namespace unsafe

    namespace detail
    {
        // no memory allocation needed
        inline void vprint(std::string_view fmt, std::format_args args)
        {
            struct {
                using value_type = char;
                void push_back(char chr) { unsafe::printc(chr); }
            } printer;

            std::vformat_to(std::back_inserter(printer), fmt, args);
        }

        template<typename ...Args> requires (sizeof...(Args) > 0)
        inline void print(std::string_view fmt, Args &&...args)
        {
            if constexpr (sizeof...(Args) == 1 && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, std::tuple<Args...>>>, std::format_args>)
                vprint(fmt, args...);
            else
                vprint(fmt, std::make_format_args(args...));
        }

        template<typename ...Args>
        inline void print(std::string_view fmt, Args &&...)
        {
            unsafe::prints(fmt);
        }
    } // namespace detail
} // namespace log

export namespace log
{
    enum class level
    {
#if ILOBILIX_DEBUG
        debug,
#endif
        info,
        warn,
        error,
        fatal
    };

    constexpr std::string_view reset_colour { "\e[0m" };
    constexpr std::string_view colours[]
    {
#if ILOBILIX_DEBUG
        "\e[90m",
#endif
        "\e[92m",
        "\e[33m",
        "\e[91m",
        "\e[41m"
    };

    constexpr std::string_view prefix[]
    {
#if ILOBILIX_DEBUG
        "debug",
#endif
        "info",
        "warn",
        "error",
        "fatal"
    };

    inline void print(std::string_view fmt, auto &&...args)
    {
        std::unique_lock _ { _lock };

        detail::print(fmt, args...);
    }

    inline void println(std::string_view fmt, auto &&...args)
    {
        std::unique_lock _ { _lock };

        detail::print(fmt, args...);
        detail::print("\n");
    }

    inline constexpr void println(level lvl, std::string_view fmt, auto &&...args)
    {
        std::unique_lock _ { _lock };

        auto index = std::to_underlying(lvl);

        auto n = get_time();
        auto [h, m, s] = lib::time_from(n / 1'000'000'000);
        n %= 1'000'000'000;
        n /= 1'000;

        detail::print("[{:02}:{:02}:{:02}.{:06}] [{}{}{}] ", h, m, s, n, colours[index], prefix[index], reset_colour);
        detail::print(fmt, args...);
        detail::print("\n");
    }

#if ILOBILIX_DEBUG
    inline constexpr void debug(std::string_view fmt, auto &&...args) { println(level::debug, fmt, args...); }
#else
    inline constexpr void debug(std::string_view, auto &&...) { }
#endif
    inline constexpr void info (std::string_view fmt, auto &&...args) { println(level::info,  fmt, args...); }
    inline constexpr void warn (std::string_view fmt, auto &&...args) { println(level::warn,  fmt, args...); }
    inline constexpr void error(std::string_view fmt, auto &&...args) { println(level::error, fmt, args...); }
    inline constexpr void fatal(std::string_view fmt, auto &&...args) { println(level::fatal, fmt, args...); }
} // export namespace log