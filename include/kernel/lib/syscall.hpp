// Copyright (C) 2022  ilobilo

#include <drivers/proc.hpp>
#include <lib/log.hpp>
#include <cerrno>
#include <array>

namespace syscall
{
    using return_type = std::pair<uintptr_t, errno_t>;
    using args_array = std::array<uintptr_t, 6>;

    template<typename>
    struct signature;

    template<typename Ret, typename ...Args>
    struct signature<Ret(Args...)>
    {
        using type = std::tuple<Args...>;
    };

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
            using type = typename signature<std::remove_cvref_t<decltype(func)>>::type;
            size_t size = std::tuple_size_v<type>;
            size_t i = 0;
            type args;

            auto [pid, tid] = proc::pid();
            log::info("syscall: [{}:{}] {}(", pid, tid, name);
            std::apply([&](auto &&...args)
            {
                // (std::invoke([&]<typename Type>(Type &arg)
                (std::invoke([&]<typename AType>(AType &)
                {
                    using Base = std::remove_cvref_t<std::remove_pointer_t<AType>>;
                    using Type = std::conditional_t<std::is_pointer_v<AType> && !std::same_as<Base, void> && !std::same_as<Base, char>, const void *, AType>;

                    auto suffix = (i >= size - 1) ? "" : ", ";
                    if constexpr (std::same_as<Type, const char *>)
                        log::print("\"{}\"{}", Type(arr[i]), suffix);
                    else
                        log::print("{}{}", Type(arr[i]), suffix);
                    i++;

                    // arg = Type(arr[i++]);
                }, args), ...);
            }, args);
            log::println(")");

            // log::infoln("syscall: {}: {}", name, args);
        }), name(_name), storage(voidptr(&func)) { }

        return_type run(args_array args) const;
    };
} // namespace syscall