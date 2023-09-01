// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <lib/math.hpp>
#include <memory>

extern "C" void *memcpy(void *dest, const void *src, size_t len);
namespace mem
{
    struct buffer
    {
        private:
        std::allocator<uint8_t> _alloc;
        uint8_t *_ptr;
        size_t _size;

        public:
        friend void swap(buffer &lhs, buffer &rhs)
        {
            using std::swap;
            swap(lhs._alloc, rhs._alloc);
            swap(lhs._ptr, rhs._ptr);
            swap(lhs._size, rhs._size);
        }

        buffer() : _alloc(), _ptr(nullptr), _size(0) { }
        buffer(size_t size) : _alloc(), _ptr(nullptr), _size(size)
        {
            this->_ptr = this->_alloc.allocate(size);
        }
        buffer(uint8_t *ptr, size_t size) : _alloc(), _ptr(nullptr), _size(size)
        {
            this->_ptr = this->_alloc.allocate(size);
            memcpy(this->_ptr, tohh(ptr), size);
        }

        buffer(buffer &&other) noexcept : _alloc(), _ptr(), _size(0) { swap(*this, other); }
        buffer &operator=(buffer &&other) noexcept { swap(*this, other); return *this; }

        buffer(const buffer &other) = delete;
        buffer &operator=(const buffer &other) = delete;

        ~buffer()
        {
            if (this->_ptr != nullptr)
                this->_alloc.deallocate(this->_ptr, this->_size);
        }

        void allocate(size_t size)
        {
            if (this->_ptr != nullptr)
                this->_alloc.deallocate(this->_ptr, this->_size);

            this->_ptr = this->_alloc.allocate(size);
            this->_size = size;
        }

        uint8_t *virt_data() { return this->_ptr; }
        uint8_t *phys_data() { return fromhh(this->_ptr); }

        const uint8_t *virt_data() const { return this->_ptr; }
        const uint8_t *phys_data() const { return fromhh(this->_ptr); }

        size_t size() const { return this->_size; }
    };
} // namespace buffer

/*
namespace buffer
{
    inline std::physical_allocator<uint8_t> physical_allocator;
    inline std::allocator<uint8_t> virtual_allocator;

    struct allocate_phys { std::physical_allocator<uint8_t> &allocator = physical_allocator; };
    struct allocate_virt { std::allocator<uint8_t> &allocator = virtual_allocator; };

    template<typename Type>
    struct base
    {
        private:
        uint8_t *_ptr;
        size_t _size;

        Type _alloc;

        public:
        friend void swap(base &lhs, base &rhs)
        {
            using std::swap;
            swap(lhs._ptr, rhs._ptr);
            swap(lhs._size, rhs._size);
            swap(lhs._alloc.allocator, rhs._alloc.allocator);
        }

        base() : _ptr(nullptr), _size(0), _alloc() { }
        base(size_t size) : _ptr(nullptr), _size(size), _alloc()
        {
            this->_ptr = this->_alloc.allocator.allocate(size);
        }
        base(uint8_t *ptr, size_t size) : _ptr(nullptr), _size(size), _alloc()
        {
            this->_ptr = this->_alloc.allocator.allocate(size);
            memcpy(this->_ptr, ptr, size);
        }

        base(base &&other) noexcept : _ptr(), _size(0), _alloc() { swap(*this, other); }
        base &operator=(base &&other) noexcept { swap(*this, other); return *this; }

        // base(base &&other) noexcept : _ptr(nullptr), _size(0), _alloc() { swap(*this, other); }
        // base &operator=(base &&other) noexcept
        // {
        //     if (this->_ptr != nullptr)
        //         this->_alloc.allocator.deallocate(this->_ptr, this->_size);

        //     this->_ptr = nullptr;
        //     this->_size = 0;

        //     swap(*this, other);
        //     return *this;
        // }

        base(const base &other) = delete;
        base &operator=(const base &other) = delete;

        // base(const base &other) noexcept { this->operator=(other); }
        // base &operator=(const base &other) noexcept
        // {
        //     if (this->_ptr != nullptr)
        //         this->_alloc.allocator.deallocate(this->_ptr, this->_size);

        //     this->_size = other._size;
        //     this->_ptr = this->_alloc.allocator.allocate(this->_size);
        //     memcpy(tohh(this->_ptr), tohh(other._ptr), this->_size);
        //     return *this;
        // }

        ~base()
        {
            if (this->_ptr != nullptr)
                this->_alloc.allocator.deallocate(this->_ptr, this->_size);
        }

        void allocate(size_t size)
        {
            if (this->_ptr != nullptr)
                this->_alloc.allocator.deallocate(this->_ptr, this->_size);

            this->_ptr = this->_alloc.allocator.allocate(size);
            this->_size = size;
        }

        uint8_t *data() { return this->_ptr; }
        const uint8_t *data() const { return this->_ptr; }

        size_t size() const { return this->_size; }
    };

    using phys = base<allocate_phys>;
    using virt = base<allocate_virt>;

    inline virt phys2virt(phys &&p)
    {
        virt ret(p.size());
        memcpy(ret.data(), tohh(p.data()), ret.size());
        return std::move(ret);
    }

    inline phys virt2phys(virt &&v)
    {
        phys ret(v.size());
        memcpy(tohh(ret.data()), v.data(), ret.size());
        return std::move(ret);
    }

    // inline virt copy_virt(virt &&p)
    // {
    //     virt ret(p.size());
    //     memcpy(ret.data(), p.data(), ret.size());
    //     return std::move(ret);
    // }

    // inline phys copy_phys(phys &&v)
    // {
    //     phys ret(v.size());
    //     memcpy(tohh(ret.data()), tohh(v.data()), ret.size());
    //     return std::move(ret);
    // }
} // namespace buffer
*/