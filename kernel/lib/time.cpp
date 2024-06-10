// Copyright (C) 2022-2024  ilobilo

#include <arch/arch.hpp>
#include <lib/time.hpp>
#include <lib/misc.hpp>
#include <vector>

namespace time
{
    timespec realtime;
    timespec monotonic;

    static std::mutex timer_lock;
    static std::vector<timer *> timers;

    timer::timer(timespec when) : when(when), event(), armed(true), fired(false)
    {
        std::unique_lock guard(timer_lock);

        timers.push_back(this);
    }

    timer::~timer()
    {
        std::unique_lock guard(timer_lock);

        if (timers.size() == 0 || this->armed == false)
            return;

        timers.erase(std::remove(timers.begin(), timers.end(), this), timers.end());
        this->armed = false;
    }

    void init()
    {
        realtime = timespec(arch::epoch(), 0);
        monotonic = timespec(arch::epoch(), 0);
    }

    void timer_handler(size_t ns)
    {
        timespec interval(0, ns);

        realtime += interval;
        monotonic += interval;

        if (timer_lock.try_lock())
        {
            for (auto tmr : timers)
            {
                if (tmr->fired)
                    continue;

                if ((tmr->when - monotonic).to_ns() == 0)
                {
                    tmr->event.trigger();
                    tmr->fired = true;
                }
            }

            timer_lock.unlock();
        }
    }

    uint64_t time_ns()
    {
        return arch::time_ns().value_or(monotonic.to_ns());
    }

    uint64_t time_ms()
    {
        return time_ns() / 1'000'000;
    }

    void nsleep(uint64_t ns)
    {
        uint64_t target = time_ns() + ns;
        while (time_ns() < target)
            arch::pause();
    }

    void msleep(uint64_t ms)
    {
        uint64_t target = time_ms() + ms;
        while (time_ms() < target)
            arch::pause();
    }
} // namespace time

void stat_t::update_time(size_t flags)
{
    if (flags & which::access)
        this->st_atim = time::realtime;
    if (flags & which::modify)
        this->st_mtim = time::realtime;
    if (flags & which::status)
        this->st_ctim = time::realtime;
}