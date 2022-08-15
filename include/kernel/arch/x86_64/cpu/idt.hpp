// Copyright (C) 2022  ilobilo

#pragma once

#include <arch/x86_64/cpu/cpu.hpp>
#include <functional>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <tuple>

namespace idt
{
    static constexpr uint8_t INT_SYSCALL = 0x69;
    static constexpr uint8_t INT_PANIC = 0xFF;

    constexpr uint8_t IRQ(uint8_t num)
    {
        return num + 0x20;
    }

    struct [[gnu::packed]] IDTEntry
    {
        uint16_t Offset1;
        uint16_t Selector;
        uint8_t IST;
        uint8_t TypeAttr;
        uint16_t Offset2;
        uint32_t Offset3;
        uint32_t Zero;

        void set(void *isr, uint8_t typeattr = 0x8E, uint8_t ist = 0);
    };

    struct [[gnu::packed]] IDTPtr
    {
        uint16_t Limit;
        uint64_t Base;

        void load()
        {
            asm volatile ("cli");
            asm volatile ("lidt %0" :: "memory"(*this));
            asm volatile ("sti");
        }
    };

    class int_handler_t
    {
        private:
        std::function<void(cpu::registers_t*)> handler;
        bool reserved = false;

        public:
        template<typename Func, typename ...Args>
        bool set(Func &&func, Args &&...args)
        {
            if (this->get())
                return false;
            this->handler = [func = std::forward<Func>(func), ...args = std::forward<Args>(args)](cpu::registers_t *regs) mutable
            {
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

        bool clear()
        {
            bool ret = this->handler;
            this->handler.clear();
            return ret;
        }

        bool get()
        {
            return bool(this->handler);
        }

        bool operator()(cpu::registers_t *regs)
        {
            if (this->get() == false)
                return false;
            this->handler(regs);
            return true;
        }
    };

    extern int_handler_t handlers[];
    extern IDTEntry idt[];
    extern IDTPtr idtr;

    std::pair<int_handler_t&, uint8_t> allocate_handler(uint8_t hint = 0);

    void mask(uint8_t irq);
    void unmask(uint8_t irq);

    void init();
} // namespace idt