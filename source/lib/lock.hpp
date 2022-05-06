// Copyright (C) 2022  ilobilo

#pragma once

#include <atomic>

class lock_t
{
    private:
    std::atomic<bool> locked = false;

    public:
    void lock()
    {
        while (__atomic_test_and_set(&this->locked, __ATOMIC_ACQUIRE));
    }
    void unlock()
    {
        __atomic_clear(&this->locked, __ATOMIC_RELEASE);
    }
    bool test()
    {
        return this->locked;
    }
};

class lock_guard
{
    private:
    lock_t *lock;
    public:
    lock_guard(lock_t &lock)
    {
        this->lock = &lock;
        this->lock->lock();
    }
    ~lock_guard()
    {
        lock->unlock();
    }
};

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define lockit(name) lock_guard CONCAT(lock##_, __COUNTER__)(name)