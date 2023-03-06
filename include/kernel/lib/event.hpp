// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <deque>
#include <span>

namespace proc { struct thread; }

// namespace simple
// {
//     struct event
//     {
//         private:
//         irq_lock lock;
//         std::deque<proc::thread*> waiters;

//         public:
//         void trigger();
//         void await();
//     };

//     template<typename Type> requires (!std::is_void_v<Type>)
//     struct promise
//     {
//         private:
//         std::aligned_storage_t<sizeof(Type), alignof(Type)> object;
//         event event;

//         public:
//         template<typename ...Args>
//         void trigger(Args &&...args)
//         {
//             new (&this->object) Type(std::forward<Args>(args)...);
//             this->event.trigger();
//         }

//         Type await()
//         {
//             this->event.await();
//             return *reinterpret_cast<Type*>(&this->object);
//         }
//     };
// } // namespace simple

namespace event
{
    struct listener
    {
        proc::thread *thread;
        size_t which;

        listener(proc::thread *thread, size_t which) : thread(thread), which(which) { }
    };

    struct event_t
    {
        irq_lock lock;
        size_t pending = 0;
        std::deque<listener> listeners;

        std::optional<size_t> await(bool block = true);
        std::optional<size_t> await_timeout(ssize_t ms, bool block = true);

        void trigger(bool drop = false);
    };

    std::optional<size_t> await(std::span<event_t*> events, bool block = true);
    std::optional<size_t> await_timeout(std::span<event_t*> events, ssize_t ms, bool block = true);

    void trigger(event_t *event, bool drop = false);
} // namespace event

using event_t = event::event_t;