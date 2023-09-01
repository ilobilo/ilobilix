// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <cstdint>
#include <cstddef>

namespace net
{
    struct checksum
    {
        private:
        uint32_t _value = 0;

        public:
        constexpr void update(uint16_t n)
        {
            this->_value += n;
            while (this->_value >> 16)
                this->_value = (this->_value >> 16) + (this->_value & 0xFFFF);
        }

        constexpr void update(const uint8_t *bytes, size_t size)
        {
            if (size & 0x01)
            {
                size--;
                this->update(uint16_t(bytes[size]) << 8);
            }

            for (size_t i = 0; i < size; i += 2)
                this->update((uint16_t(bytes[i]) << 8) | bytes[i + 1]);
        }

        constexpr uint16_t operator()(const uint8_t *data, size_t size)
        {
            this->update(data, size);
            return this->value();
        }

        constexpr uint16_t value() { return ~this->_value; }
        constexpr void reset() { this->_value = 0; }

        static constexpr uint16_t compute(const uint8_t *data, size_t size)
        {
            checksum cs;
            return cs(data, size);
        }
    };
} // namespace net