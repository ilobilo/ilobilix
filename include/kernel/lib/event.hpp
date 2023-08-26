// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <mutex>
#include <deque>
#include <span>

namespace proc { struct thread; }
namespace event
{
    namespace simple
    {
        struct event_t
        {
            std::atomic_size_t triggered = 0;
            // std::atomic_bool triggered = false;
            std::mutex lock;

            void await();
            bool await_timeout(size_t ms);
            void trigger(bool drop = false);

            event_t() = default;
        };

        struct alt_event_t
        {
            std::atomic_size_t triggered = 0;
            std::atomic_size_t awaiters = 0;
            std::mutex lock;

            void await();
            bool await_timeout(size_t ms);
            void trigger(bool drop = false);

            alt_event_t() = default;
        };
    } // namespace simple

    struct listener
    {
        proc::thread *thread;
        size_t which;

        listener(proc::thread *thread, size_t which) : thread(thread), which(which) { }
    };

    struct event_t
    {
        std::mutex lock;
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