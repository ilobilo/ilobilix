// Copyright (C) 2022  ilobilo

#pragma once

#include <frg/mutex.hpp>
#include <arch/arch.hpp>
#include <atomic>

struct ticket_lock
{
    private:
    uint64_t _next_ticket;
    uint64_t _serving_ticket;

    public:
    constexpr ticket_lock() : _next_ticket(0), _serving_ticket(0) { }

    ticket_lock(const ticket_lock &) = delete;
    ticket_lock &operator=(const ticket_lock &) = delete;

    void lock()
    {
        auto ticket = __atomic_fetch_add(&this->_next_ticket, 1, __ATOMIC_RELAXED);
        while (__atomic_load_n(&this->_serving_ticket, __ATOMIC_ACQUIRE) != ticket)
            arch::pause();
    }

    void unlock()
    {
        auto current = __atomic_load_n(&this->_serving_ticket, __ATOMIC_RELAXED);
        __atomic_store_n(&this->_serving_ticket, current + 1, __ATOMIC_RELEASE);
    }

    bool is_locked()
    {
        auto current = __atomic_load_n(&this->_serving_ticket, __ATOMIC_RELAXED);
        auto next = __atomic_load_n(&this->_next_ticket, __ATOMIC_RELAXED);
        return current != next;
    }

    bool try_lock()
    {
        if (this->is_locked())
            return false;

        this->lock();
        return true;
    }
};

struct normal_lock
{
    private:
    std::atomic_flag _locked;

    public:
    constexpr normal_lock() : _locked() { }

    normal_lock(const normal_lock &) = delete;
    normal_lock &operator=(const normal_lock &) = delete;

    void lock()
    {
        while (this->_locked.test_and_set(std::memory_order_acquire) == false)
            arch::pause();
    }

    void unlock()
    {
        this->_locked.clear(std::memory_order_release);
    }

    bool is_locked()
    {
        return this->_locked.test(std::memory_order_release);
    }

    bool try_lock()
    {
        if (this->is_locked())
            return false;

        this->lock();
        return true;
    }
};

using lock_t = ticket_lock;

struct irq_lock
{
    private:
    bool _irqs = false;
    lock_t _lock;

    public:
    constexpr irq_lock() : _irqs(false), _lock() { }

    irq_lock(const irq_lock &) = delete;
    irq_lock &operator=(const irq_lock &) = delete;

    void lock()
    {
        bool irqs = arch::int_status();
        arch::int_toggle(false);

        this->_lock.lock();
        this->_irqs = irqs;
    }

    void unlock()
    {
        bool irqs = this->_irqs;
        this->_lock.unlock();

        if (irqs == true)
            arch::int_toggle(true);
        else
            arch::int_toggle(false);
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