// Copyright (C) 2022-2024  ilobilo

#include <misc/cxxabi.hpp>
#include <lib/panic.hpp>
#include <lib/hash.hpp>
#include <lib/log.hpp>
#include <cxxabi.h>
#include <cstdint>

extern "C"
{
    static constexpr uint64_t atexit_max_funcs = 128;

    struct {
        void (*destructor_func)(void*);
        void *obj_ptr;
        void *dso_handle;
    }  __atexit_funcs[atexit_max_funcs];

    unsigned __atexit_func_count = 0;
    void *__dso_handle = nullptr;

    int __cxa_atexit(void (*func)(void *), void *objptr, void *dso)
    {
        if (__atexit_func_count >= atexit_max_funcs)
            return -1;

        __atexit_funcs[__atexit_func_count].destructor_func = func;
        __atexit_funcs[__atexit_func_count].obj_ptr = objptr;
        __atexit_funcs[__atexit_func_count].dso_handle = dso;
        __atexit_func_count++;

        return 0;
    };

    void __cxa_finalize(void *func)
    {
        auto i = __atexit_func_count;
        if (func == nullptr)
        {
            while (i--)
            {
                if (__atexit_funcs[i].destructor_func)
                    (*__atexit_funcs[i].destructor_func)(__atexit_funcs[i].obj_ptr);
            }
            return;
        }

        while (i--)
        {
            if (__atexit_funcs[i].destructor_func == func)
            {
                (*__atexit_funcs[i].destructor_func)(__atexit_funcs[i].obj_ptr);
                __atexit_funcs[i].destructor_func = 0;
            };
        };
    };

    void __cxa_pure_virtual()
    {
        PANIC("__cxa_pure_virtual()");
    }

    namespace __cxxabiv1
    {
        int __cxa_guard_acquire(__guard *guard)
        {
            if ((*guard) & 0x0001)
                return 0;
            if ((*guard) & 0x0100)
                abort();

            *guard |= 0x0100;
            return 1;
        }

        void __cxa_guard_release(__guard *guard)
        {
            *guard |= 0x0001;
        }

        void __cxa_guard_abort(__guard *guard)
        {
            PANIC("__cxa_guard_abort({})", static_cast<void*>(guard));
        }
    } // namespace __cxxabiv1

    uintptr_t __stack_chk_guard = 0x595E9FBD94FDA766;
    [[noreturn]] void __stack_chk_fail()
    {
        PANIC("stack smashing detected!");
    }
} // extern "C"

namespace std
{
    [[gnu::noreturn]] void terminate() noexcept
    {
        PANIC("std::terminate()");
    }

    // For libstdc++
    size_t _Hash_bytes(const void *key, size_t len, size_t seed)
    {
        return MurmurHash2_64A(key, len, seed);
    }

    size_t _Fnv_hash_bytes(const void *key, size_t len, size_t seed)
    {
        return FNV1a(key, len, seed);
    }
} // namespace std

namespace cxxabi
{
    extern "C" void (*__init_array_start[])();
    extern "C" void (*__init_array_end[])();

    void init()
    {
        log::infoln("Running global constructors...");
        for (auto ctor = __init_array_start; ctor < __init_array_end; ctor++)
            (*ctor)();
    }
} // namespace cxxabi