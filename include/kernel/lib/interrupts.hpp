// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <cpu/cpu.hpp>
#include <functional>
#include <utility>

namespace interrupts
{
    class handler
    {
        private:
        std::function<void(cpu::registers_t*)> handler;
        bool reserved = false;

        public:
        bool eoi_first = false;

        template<typename Func, typename ...Args>
        bool set(Func &&func, Args &&...args)
        {
            if (this->used())
                return false;

            this->handler = [func = std::forward<Func>(func), ...args = std::forward<Args>(args)](cpu::registers_t *regs) mutable {
                func(regs, args...);
            };
            return true;
        }

        bool is_reserved()
        {
            return this->reserved == true;
        }

        bool reserve()
        {
            if (this->is_reserved())
                return false;

            return this->reserved = true;
        }

        bool reset()
        {
            bool ret = static_cast<bool>(this->handler);
            this->handler.clear();
            this->reserved = false;
            return ret;
        }

        bool used()
        {
            return bool(this->handler);
        }

        bool operator()(cpu::registers_t *regs)
        {
            if (this->used() == false)
                return false;

            this->handler(regs);
            return true;
        }
    };

    std::pair<handler&, size_t> allocate_handler();
    handler &get_handler(size_t vector);
} // namespace interrupts