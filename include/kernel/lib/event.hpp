// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <deque>

namespace proc { struct thread; }
struct event
{
    private:
    irq_lock lock;
    std::deque<proc::thread*> waiters;

    public:
    void trigger();
    void await();
};

template<typename Type> requires (!std::is_void_v<Type>)
struct promise
{
    private:
    std::aligned_storage_t<sizeof(Type), alignof(Type)> object;
    event event;

    public:
    template<typename ...Args>
    void trigger(Args &&...args)
    {
        new ((Type*)object.data) Type(std::forward<Args>(args)...);
        event.trigger();
    }

    // TODO: Reference?
    Type await()
    {
        event.await();
        return *reinterpret_cast<Type*>(object.data);
    }
};