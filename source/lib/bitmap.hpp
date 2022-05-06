// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/string.hpp>
#include <cstddef>
#include <cstdint>

struct Bitmap
{
    uint8_t *buffer = nullptr;
    size_t size = 0;

    void setbuffer(uint8_t *buffer, size_t size)
    {
        this->buffer = buffer;
        this->size = size;
    }
    bool operator[](size_t index)
    {
        return this->get(index);
    }
    void operator()(uint8_t value)
    {
        memset(this->buffer, value, this->size);
    }

    bool get(size_t index)
    {
        uint64_t bytei = index / 8;
        uint8_t biti = index % 8;
        uint8_t bitindexer = 0b10000000 >> biti;
        if (buffer[bytei] & bitindexer) return true;
        return false;
    }
    bool set(size_t index, bool value)
    {
        uint64_t bytei = index / 8;
        uint8_t biti = index % 8;
        uint8_t bitindexer = 0b10000000 >> biti;

        bool oldval = buffer[bytei] & bitindexer;

        buffer[bytei] &= ~bitindexer;
        if (value) buffer[bytei] |= bitindexer;

        return oldval;
    }
};

