// Copyright (C) 2024  ilobilo

export module lib:lock;
import std;

namespace lib
{
    template<bool ints>
    class spinlock_base
    {
        private:
        std::atomic_size_t _next_ticket;
        std::atomic_size_t _serving_ticket;
        bool _interrupts;

        void arch_lock();
        void arch_unlock() const;
        void arch_pause() const;

        public:
        constexpr spinlock_base()
            : _next_ticket { 0 }, _serving_ticket { 0 }, _interrupts { false } { }

        spinlock_base(const spinlock_base &) = delete;
        spinlock_base &operator=(const spinlock_base &) = delete;

        void lock()
        {
            auto ticket = _next_ticket.fetch_add(1, std::memory_order_relaxed);
            while (_serving_ticket.load(std::memory_order_acquire) != ticket)
                arch_pause();

            if constexpr (ints)
                arch_lock();
        }

        void unlock()
        {
            if (is_locked() == false)
                return;

            if constexpr (ints)
                arch_unlock();

            auto current = _serving_ticket.load(std::memory_order_relaxed);
            _serving_ticket.store(current + 1, std::memory_order_release);
        }

        bool is_locked() const
        {
            auto current = _serving_ticket.load(std::memory_order_relaxed);
            auto next = _next_ticket.load(std::memory_order_relaxed);
            return current != next;
        }

        bool try_lock()
        {
            if (is_locked())
                return false;

            lock();
            return true;
        }
    };

    export using spinlock = spinlock_base<true>;
    export using spinlock_noints = spinlock_base<false>;
} // export namespace lib