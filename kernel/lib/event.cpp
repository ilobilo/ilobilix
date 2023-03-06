// Copyright (C) 2022-2023  ilobilo

#include <drivers/proc.hpp>
#include <lib/event.hpp>

// namespace simple
// {
//     void event::trigger()
//     {
//         lockit(this->lock);

//         while (this->waiters.empty() == false)
//         {
//             auto thread = this->waiters.pop_front_element();
//             proc::unblock(thread);
//         }
//     }

//     void event::await()
//     {
//         lockit(this->lock);

//         auto thread = this_thread();
//         this->waiters.push_back(thread);

//         proc::block(thread);
//     }
// } // namespace simple

namespace event
{
    struct events_guard
    {
        std::span<event_t*> _events;
        lock_t _lock;

        events_guard(std::span<event_t*> events) : _events(events) { this->lock(); }
        ~events_guard() { this->unlock(); }

        void lock()
        {
            if (this->_lock.is_locked() == true)
                return;

            this->_lock.lock();
            for (auto &event : this->_events)
                event->lock.lock();
        }

        void unlock()
        {
            if (this->_lock.is_locked() == false)
                return;

            this->_lock.unlock();
            for (auto &event : this->_events)
                event->lock.unlock();
        }
    };

    static std::optional<size_t> pending(std::span<event_t*> events)
    {
        for (size_t i = 0; auto &event : events)
        {
            if (event->pending > 0)
            {
                event->pending--;
                return i;
            }
            i++;
        }
        return std::nullopt;
    }

    static void attach(std::span<event_t*> events, proc::thread *thread, ssize_t timeout = -1)
    {
        thread->events.clear();
        thread->timeout = timeout > 0 ? time::time_ms() + timeout : -1;

        for (size_t i = 0; const auto &event : events)
        {
            event->listeners.emplace_back(thread, i);
            thread->events.push_back(event);
            i++;
        }
    }

    static void detach(proc::thread *thread)
    {
        for (const auto event : thread->events)
        {
            for (auto it = event->listeners.begin(); it < event->listeners.end(); it++)
            {
                if (it->thread != thread)
                    continue;
                it = event->listeners.erase(it);
            }
        }
        thread->events.clear();
    }

    std::optional<size_t> await(std::span<event_t*> events, bool block)
    {
        auto thread = this_thread();
        events_guard guard(events);

        auto i = pending(events);
        if (i.has_value())
            return i;

        if (block == false)
            return std::nullopt;

        attach(events, thread);

        guard.unlock();
        proc::block();

        return thread->event;
    }

    std::optional<size_t> await_timeout(std::span<event_t*> events, ssize_t ms, bool block)
    {
        auto thread = this_thread();
        events_guard guard(events);

        auto i = pending(events);
        if (i.has_value())
            return i;

        if (block == false)
            return std::nullopt;

        attach(events, thread, ms);

        guard.unlock();
        proc::block();

        if (thread->timeout == 0)
        {
            guard.lock();
            detach(thread);
            return std::nullopt;
        }

        return thread->event;
    }

    void trigger(event_t *event, bool drop)
    {
        lockit(event->lock);
        if (event->listeners.size() == 0)
        {
            if (drop == false)
                event->pending++;
            return;
        }

        for (const auto &listener : event->listeners)
        {
            auto thread = listener.thread;
            thread->event = listener.which;
            proc::unblock(thread);
        }
        event->listeners.clear();
    }

    std::optional<size_t> event_t::await(bool block)
    {
        std::array evs { this };
        return event::await(evs, block);
    }

    std::optional<size_t> event_t::await_timeout(ssize_t ms, bool block)
    {
        std::array evs { this };
        return event::await_timeout(evs, ms, block);
    }

    void event_t::trigger(bool drop)
    {
        event::trigger(this, drop);
    }
} // namespace event