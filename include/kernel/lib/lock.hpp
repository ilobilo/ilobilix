// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <frg/mutex.hpp>
#include <arch/arch.hpp>
#include <lib/types.hpp>
#include <cassert>
#include <atomic>

struct ticket_lock
{
    private:
    std::atomic_size_t _next_ticket;
    std::atomic_size_t _serving_ticket;

    public:
    constexpr ticket_lock() : _next_ticket(0), _serving_ticket(0) { }

    ticket_lock(const ticket_lock &) = delete;
    ticket_lock &operator=(const ticket_lock &) = delete;

    void lock()
    {
        auto ticket = this->_next_ticket.fetch_add(1, std::memory_order_relaxed);
        while (this->_serving_ticket.load(std::memory_order_acquire) != ticket)
            arch::pause();
    }

    void unlock()
    {
        if (this->is_locked() == false)
            return;

        auto current = this->_serving_ticket.load(std::memory_order_relaxed);
        this->_serving_ticket.store(current + 1, std::memory_order_release);
    }

    bool is_locked()
    {
        auto current = this->_serving_ticket.load(std::memory_order_relaxed);
        auto next = this->_next_ticket.load(std::memory_order_relaxed);
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

        arch::int_toggle(irqs);
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

namespace proc { void yield(); std::pair<pid_t, tid_t> pid(); }
struct smart_lock
{
    private:
    std::optional<std::pair<pid_t, tid_t>> _owner;
    std::atomic<size_t> _refcount;
    lock_t _lock;

    public:
    constexpr smart_lock() : _owner(std::nullopt), _refcount(0), _lock() { }

    smart_lock(const smart_lock &) = delete;
    smart_lock &operator=(const smart_lock &) = delete;

    void lock()
    {
        auto pid = proc::pid();
        frg::unique_lock<lock_t> guard(this->_lock);
        if (this->_owner != pid)
        {
            while (this->_owner.has_value())
                proc::yield();
            this->_owner = pid;
        }
        this->_refcount++;
    }

    bool is_locked()
    {
        return this->_owner != proc::pid();
    }

    bool try_lock()
    {
        if (this->is_locked())
            return false;

        this->lock();
        return true;
    }

    void unlock()
    {
        frg::unique_lock<lock_t> guard(this->_lock);
        if (this->_owner != proc::pid() || this->_refcount == 0)
            assert(!"invalid unlock");

        if (this->_refcount-- == 1)
            this->_owner = std::nullopt;
    }
};

#define CONCAT_IMPL(x, y) x ## y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define lockit(name) frg::unique_lock<decltype(name)> CONCAT(lock ## _, __COUNTER__)(name)