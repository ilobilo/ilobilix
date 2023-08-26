// Copyright (C) 2022-2023  ilobilo

#include <drivers/proc.hpp>
#include <lib/event.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>

namespace event
{
    namespace simple
    {
        void event_t::await()
        {
            std::unique_lock guard(this->lock);

            while (this->triggered.load(std::memory_order_acquire) == 0)
                arch::pause();

            this->triggered.fetch_sub(1, std::memory_order_seq_cst);

            // while (this->triggered.load(std::memory_order_acquire) == false)
            //     arch::pause();

            // this->triggered.store(false, std::memory_order_seq_cst);
        }

        bool event_t::await_timeout(size_t ms)
        {
            std::unique_lock guard(this->lock);

            auto start = time::time_ms();
            auto end = start + ms;

            while (start < end)
            {
                if (this->triggered.load(std::memory_order_acquire) > 0)
                    break;

                // if (this->triggered.load(std::memory_order_acquire) == false)
                //     break;

                arch::pause();
                start++;
            }
            if (start >= end)
                return false;

            this->triggered.fetch_sub(1, std::memory_order_seq_cst);
            // this->triggered.store(false, std::memory_order_seq_cst);

            return true;
        }

        void event_t::trigger(bool drop)
        {
            if (this->lock.is_locked() == false && drop == true)
                return;

            this->triggered.fetch_add(1, std::memory_order_release);
            // this->triggered.store(true, std::memory_order_release);
        }

        void alt_event_t::await()
        {
            this->lock.lock();
                this->awaiters++;
            this->lock.unlock();

            while (this->triggered.load(std::memory_order_acquire) == 0)
                arch::pause();

            std::unique_lock guard(this->lock);
            if (--this->awaiters == 0)
                this->triggered.fetch_sub(1, std::memory_order_seq_cst);
        }

        bool alt_event_t::await_timeout(size_t ms)
        {
            this->lock.lock();
                this->awaiters++;
            this->lock.unlock();

            auto start = time::time_ms();
            auto end = start + ms;

            while (start < end)
            {
                if (this->triggered.load(std::memory_order_acquire) > 0)
                    break;

                arch::pause();
                start++;
            }

            std::unique_lock guard(this->lock);
            this->awaiters--;

            if (start >= end)
                return false;

            if (this->awaiters == 0)
                this->triggered.fetch_sub(1, std::memory_order_seq_cst);

            return true;
        }

        void alt_event_t::trigger(bool drop)
        {
            std::unique_lock guard(this->lock);
            if (this->awaiters == 0 && drop == true)
                return;

            this->triggered.fetch_add(1, std::memory_order_release);
        }
    } // namespace simple

    struct events_guard
    {
        std::span<event_t*> _events;
        irq_lock _lock;

        events_guard(std::span<event_t*> events) : _events(events), _lock() { this->lock(); }
        ~events_guard() { this->unlock(); }

        void lock()
        {
            this->_lock.try_lock();
            for (auto &event : this->_events)
                event->lock.lock();
        }

        void unlock()
        {
            if (this->_lock.is_locked() == false)
                return;

            for (auto &event : this->_events)
                event->lock.unlock();

            this->_lock.unlock();
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
        guard.lock();

        detach(thread);

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
        guard.lock();

        detach(thread);

        if (thread->timeout == 0)
            return std::nullopt;

        return thread->event;
    }

    void trigger(event_t *event, bool drop)
    {
        std::array evs { event };
        events_guard guard(evs);

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