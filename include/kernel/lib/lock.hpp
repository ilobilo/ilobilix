// Copyright (C) 2022  ilobilo

#pragma once

#include <frg/mutex.hpp>
#include <arch/arch.hpp>

struct lock_t
{
    private:
    uint64_t _next_ticket;
    uint64_t _serving_ticket;

    public:
    constexpr lock_t() : _next_ticket(0), _serving_ticket(0) { }

    lock_t(const lock_t &) = delete;
    lock_t &operator= (const lock_t &) = delete;

    void lock()
    {
        auto ticket = __atomic_fetch_add(&this->_next_ticket, 1, __ATOMIC_RELAXED);
        while(__atomic_load_n(&this->_serving_ticket, __ATOMIC_ACQUIRE) != ticket)
            arch::pause();
    }

    void unlock()
    {
        auto current = __atomic_load_n(&this->_serving_ticket, __ATOMIC_RELAXED);
        __atomic_store_n(&this->_serving_ticket, current + 1, __ATOMIC_RELEASE);
    }

    bool is_locked()
    {
        return __atomic_load_n(&this->_serving_ticket, __ATOMIC_ACQUIRE) == __atomic_load_n(&this->_next_ticket, __ATOMIC_RELAXED);
    }

    bool try_lock()
    {
        if (this->is_locked())
            return false;

        this->lock();
        return true;
    }
};

struct irq_lock_t
{
    private:
    bool _irqs = false;
    lock_t _lock;

    public:
    constexpr irq_lock_t() : _irqs(false), _lock() { }

    irq_lock_t(const irq_lock_t &) = delete;
    irq_lock_t &operator=(const irq_lock_t &) = delete;

    void lock()
    {
        bool irqs = arch::int_status();
        arch::int_switch(false);

        this->_lock.lock();
        this->_irqs = irqs;
    }

    void unlock()
    {
        bool irqs = this->_irqs;
        this->_lock.unlock();

        if (irqs == true)
            arch::int_switch(true);
        else
            arch::int_switch(false);
    }

    bool is_locked()
    {
        return this->_lock.is_locked();
    }

    bool try_lock()
    {
        if (this->is_locked())
            return false;

        this->lock();
        return true;
    }
};

#define CONCAT_IMPL(x, y) x ## y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define lockit_unique(name) frg::unique_lock<typeof(name)> CONCAT(lock##_, __COUNTER__)(name)
#define lockit_shared(name) frg::shared_lock<typeof(name)> CONCAT(lock##_, __COUNTER__)(name)

#define lockit(name) lockit_unique(name)