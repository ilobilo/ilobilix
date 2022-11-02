// Copyright (C) 2022  ilobilo

#pragma once

#include <cstddef>
#include <cstdint>

struct bitmap_t
{
    uint8_t *buffer = nullptr;
    size_t size = 0;

    constexpr bitmap_t() = default;
    constexpr bitmap_t(uint8_t *buffer, size_t size) : buffer(buffer), size(size) { };

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
        return bit(*this, index);
    }

    constexpr bool get(size_t index)
    {
        return this->buffer[index / 8] & (1 << (index % 8));
    }

    constexpr bool set(size_t index, bool value)
    {
        bool ret = this->get(index);

        if (value == true)
            this->buffer[index / 8] |= (1 << (index % 8));
        else
            this->buffer[index / 8] &= ~(1 << (index % 8));

        return ret;
    }
};

