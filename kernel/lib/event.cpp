// Copyright (C) 2022  ilobilo

#include <drivers/proc.hpp>
#include <lib/event.hpp>

void event::trigger()
{
    lockit(this->lock);

    while (this->waiters.empty() == false)
    {
        auto thread = this->waiters.pop_front_element();
        proc::unblock(thread);
    }
}

void event::await()
{
    lockit(this->lock);

    auto thread = this_thread();
    this->waiters.push_back(thread);

    proc::block(thread);
}