// Copyright (C) 2022  ilobilo

#include <drivers/proc.hpp>
#include <fmt/ranges.h>
#include <lib/log.hpp>
#include <cerrno>
#include <array>

namespace syscall
{
    using args_array = std::array<uintptr_t, 6>;

    template<typename>
    struct signature;

    template<typename Ret, typename ...Args>
    struct signature<Ret(Args...)>
    {
        using type = std::tuple<Args...>;
    };

    // template<typename T, typename Base = std::remove_cvref_t<std::remove_pointer_t<T>>>
    // using to_formattable_ptr = typename std::conditional_t<std::is_pointer_v<T> && !std::same_as<Base, void> && !std::same_as<Base, char>, const void *, T>;

    template<typename Type>
    using to_formattable_ptr = typename std::conditional_t<std::is_pointer_v<Type> && !fmt::is_formattable<Type>::value, const void *, Type>;

    template<typename ...Ts>
    auto ptr(const std::tuple<Ts...> &tup)
    {
        return std::tuple<to_formattable_ptr<Ts>...>(tup);
    }

    class wrapper
    {
        using voidptr = void*;

        private:
        void (*print)(args_array arr, const char *name);
        const char *name;
        voidptr storage;

        public:
        constexpr wrapper(const char *_name, const auto &func) : print([](args_array arr, const char *name)
        {
            typename signature<std::remove_cvref_t<decltype(func)>>::type args;
            size_t i = 0;

            std::apply([&](auto &&...args)
            {
                (std::invoke([&]<typename Type>(Type &arg)
                {
                    arg = Type(arr[i++]);
                }, args), ...);
            }, args);

            auto [pid, tid] = proc::pid();
            log::infoln("syscall: [{}:{}] {}{}", pid, tid, name, ptr(args));
        }), name(_name), storage(voidptr(&func)) { }

        uintptr_t run(args_array args) const;
    };
} // namespace syscall