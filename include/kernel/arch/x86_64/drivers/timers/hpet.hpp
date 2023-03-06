// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <arch/x86_64/cpu/cpu.hpp>
#include <drivers/acpi.hpp>
#include <lib/lock.hpp>
#include <functional>
#include <cstdint>
#include <array>

namespace timers::hpet
{
    enum modes
    {
        PERIODIC,
        ONESHOT
    };

    struct [[gnu::packed]] HPET
    {
        uint64_t cap;
        uint64_t reserved;
        uint64_t cmd;
        uint64_t reserved2;
        uint64_t ist;
        uint64_t reserved3[25];
        uint64_t main_counter;
        uint64_t reserved4;
        struct [[gnu::packed]]
        {
            uint64_t cmd;
            uint64_t val;
            uint64_t fsb;
            uint64_t reserved;
        } comparators[];
    };

    class device;
    class comparator
    {
        friend class device;
        private:
        irq_lock lock;

        device *_device;
        uint8_t _num;

        bool _fsb;
        bool _periodic;

        uint32_t _int_route;
        uint8_t _vector;

        enum int_modes
        {
            INT_FSB,
            INT_IOAPIC,
            INT_LEGACY,
            INT_NONE
        };
        int_modes _int_mode = INT_NONE;

        modes _mode;

        std::function<void()> _func;

        void start_timer_internal(uint64_t ns);

        public:
        template<typename Func, typename ...Args>
        bool start_timer(uint64_t ns, modes mode, Func &&func, Args &&...args)
        {
            if (this->_int_mode == INT_NONE)
                return false;

            lockit(this->lock);

            if (mode == PERIODIC && this->_periodic == false)
                return false;

            if (this->_func != false)
                return false;

            this->_mode = mode;
            this->_func = [func = std::forward<Func>(func), ...args = std::forward<Args>(args)] mutable
            {
                func(args...);
            };

            this->start_timer_internal(ns);
            return true;
        }

        inline bool supports_periodic()
        {
            return this->_periodic;
        }
        void cancel_timer();
    };

    class device
    {
        friend class comparator;
        private:
        volatile HPET *regs;

        uint8_t comp_count;
        uint64_t clk;

        bool _legacy;

        std::array<comparator, 32> comps;

        void start();
        void stop();

        public:
        device(acpi::HPETHeader *table);

        void nsleep(uint64_t ns);
        void msleep(uint64_t ms);

        uint64_t time_ns();
        uint64_t time_ms();
    };

    extern std::vector<comparator*> comparators;
    extern std::vector<device*> devices;

    extern bool initialised;

    void nsleep(uint64_t ns);
    void msleep(uint64_t ms);

    uint64_t time_ns();
    uint64_t time_ms();

    inline irq_lock lock;

    template<typename Func, typename ...Args>
    static comparator *start_timer(uint64_t ns, modes mode, Func &&func, Args &&...args)
    {
        lockit(lock);

        for (auto &comp : comparators)
        {
            if (mode == PERIODIC && comp->supports_periodic() == false)
                continue;
            if (comp->start_timer(ns, mode, func, args...))
                return comp;
        }

        return nullptr;
    }

    void cancel_timer(comparator *comp);
    void init();
} // namespace timers::hpet