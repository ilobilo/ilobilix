// Copyright (C) 2022-2024  ilobilo

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
        bool load_lh = false;

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

        bool is_reserved() const
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
            return ret;
        }

        bool reset_all()
        {
            this->reserved = false;
            return this->reset();
        }

        bool used() const
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

    std::pair<handler &, size_t> allocate_handler(size_t hint = 0);
    handler &get_handler(size_t vector);

    void mask(size_t vector);
    void unmask(size_t vector);
} // namespace interrupts