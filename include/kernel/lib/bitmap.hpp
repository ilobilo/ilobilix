// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <lib/math.hpp>
#include <cstddef>
#include <cassert>
#include <new>

extern "C" void *memset(void *dest, int ch, size_t len);
struct bitmap_t
{
    private:
    uint8_t *buffer = nullptr;
    size_t size = 0;

    bool _initialised = false;
    bool allocated = false;

    public:
    static constexpr bool available = false;
    static constexpr bool used = true;

    constexpr bitmap_t(uint8_t *buffer, size_t size) : buffer(buffer), size(size), _initialised(true) { };
    constexpr bitmap_t(size_t size)
    {
        this->allocate(size);
    };
    constexpr bitmap_t() = default;

    constexpr ~bitmap_t()
    {
        if (this->allocated == true)
            delete[] this->buffer;
    }

    void allocate(size_t size)
    {
        assert(this->_initialised == false);

        auto count = div_roundup(size, 8);
        this->buffer = new uint8_t[count]();
        this->size = size;

        this->_initialised = true;
        this->allocated = true;
    }

    void initialise(uint8_t *buffer, size_t size, bool clear = true)
    {
        assert(this->_initialised == false);

        if (clear == true)
            this->buffer = new (buffer) uint8_t[div_roundup(size, 8)]();
        else
            this->buffer = buffer;
        this->size = size;

        this->_initialised = true;
    }

    struct bit
    {
        bitmap_t &parent;
        size_t index;

        constexpr bit(bitmap_t &parent, size_t index) : parent(parent), index(index) { }

        constexpr void operator=(bool value)
        {
            this->parent.set(this->index, value);
        }

        constexpr operator bool() const
        {
            return this->parent.get(this->index);
        }
    };

    constexpr bit operator[](size_t index)
    {
        assert(this->_initialised == true);
        return bit(*this, index);
    }

    constexpr bool get(size_t index)
    {
        assert(this->_initialised == true);
        return this->buffer[index / 8] & (1 << (index % 8));
    }

    constexpr bool set(size_t index, bool value)
    {
        assert(this->_initialised == true);
        bool ret = this->get(index);

        if (value == true)
            this->buffer[index / 8] |= (1 << (index % 8));
        else
            this->buffer[index / 8] &= ~(1 << (index % 8));

        return ret;
    }

    constexpr size_t length() const
    {
        return this->_initialised ? size : 0;
    }

    constexpr uint8_t *data() const
    {
        return this->buffer;
    }

    constexpr bool initialised() const
    {
        return this->_initialised;
    }
};