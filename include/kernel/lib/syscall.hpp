// Copyright (C) 2022-2024  ilobilo

#include <drivers/proc.hpp>
#include <fmt/ranges.h>
#include <lib/log.hpp>
#include <cerrno>
#include <array>

#include <magic_enum_format.hpp>
#include <magic_enum.hpp>

namespace syscall
{
    template<typename>
    struct signature;

    template<typename Ret, typename ...Args>
    struct signature<Ret(Args...)>
    {
        using type = Ret(Args...);
        using args = std::tuple<Args...>;
        using ret = Ret;
    };

    struct nullable_string
    {
        const char *str;
        explicit constexpr nullable_string(const char *s) : str { s } { }
    };
    inline constexpr auto format_as(nullable_string n) { return n.str ?: "(null)"; }

    template<typename Type>
    using to_formattable_ptr =
        typename std::conditional_t<
            std::is_pointer_v<Type>,
            std::conditional_t<
                std::is_constructible_v<
                    std::string_view,
                    Type
                >,
                nullable_string,
                const void *
            >, Type
        >;

    template<typename ...Ts>
    auto ptr(const std::tuple<Ts...> &tup)
    {
        return std::tuple<to_formattable_ptr<Ts>...>(tup);
    }

    class wrapper
    {
        private:
        uintptr_t (*entry)(void *func, std::array<uintptr_t, 6> arr, const char *name, bool (*check_func)(uintptr_t));
        bool (*check_func)(uintptr_t ret);
        const char *name;
        void *storage;

        public:
        constexpr wrapper(const char *_name, const auto &func, bool (*check_func)(uintptr_t) = [](uintptr_t ret) { return intptr_t(ret) < 0; })
            : entry([](void *storage, std::array<uintptr_t, 6> arr, const char *name, bool (*check_func)(uintptr_t)) -> uintptr_t
            {
                using sign = signature<std::remove_cvref_t<decltype(func)>>;
                typename sign::args args;
                size_t i = 0;

                std::apply([&](auto &&...args)
                {
                    (std::invoke([&]<typename Type>(Type &arg)
                    {
                        arg = Type(arr[i++]);
                    }, args), ...);
                }, args);

#if SYSCALL_DEBUG
                auto [pid, tid] = proc::pid();
                log::infoln("syscall: [{}:{}] {}{}", pid, tid, name, ptr(args));
#endif

                errno = no_error;
                auto ret = std::apply(reinterpret_cast<typename sign::type*>(storage), args);

                auto val = magic_enum::enum_cast<errno_t>(errno);
                if (val.has_value() && check_func(uintptr_t(ret)))
                {
#if SYSCALL_DEBUG
                    log::infoln("syscall: [{}:{}] {} -> {}", pid, tid, name, val.value());
#endif
                    return -intptr_t(val.value());
                }
#if SYSCALL_DEBUG
                else log::infoln("syscall: [{}:{}] {} -> {}", pid, tid, name, ret);
#endif
                return uintptr_t(ret);
            }), check_func(check_func), name(_name), storage((void*)(&func)) { }

        uintptr_t run(std::array<uintptr_t, 6> args) const
        {
            return this->entry(this->storage, args, this->name, this->check_func);
        }
    };
} // namespace syscall