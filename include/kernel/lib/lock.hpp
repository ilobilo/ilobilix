// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <arch/arch.hpp>
#include <lib/types.hpp>
#include <cassert>
#include <utility>
#include <atomic>
#include <mutex>

struct irq_lock
{
    private:
    bool _irqs = false;
    std::mutex _lock;

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
    std::mutex _lock;

    public:
    constexpr smart_lock() : _owner(std::nullopt), _refcount(0), _lock() { }

    smart_lock(const smart_lock &) = delete;
    smart_lock &operator=(const smart_lock &) = delete;

    void lock()
    {
        auto pid = proc::pid();
        std::unique_lock<std::mutex> guard(this->_lock);
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
        std::unique_lock<std::mutex> guard(this->_lock);
        if (this->_owner != proc::pid() || this->_refcount == 0)
            assert(!"invalid unlock");

        if (this->_refcount-- == 1)
            this->_owner = std::nullopt;
    }
};

// #define CONCAT_IMPL(x, y) x ## y
// #define CONCAT(x, y) CONCAT_IMPL(x, y)

// #define lockit(name) std::unique_lock<decltype(name)> CONCAT(lock ## _, __COUNTER__)(name)