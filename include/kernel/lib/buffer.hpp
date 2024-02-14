// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <lib/math.hpp>
#include <memory>

extern "C" void *memcpy(void *dest, const void *src, size_t len);
namespace mem
{
    template<typename Type, typename Allocator = std::allocator<Type>, bool Arr = true>
    struct buffer
    {
        private:
        Allocator _alloc;
        Type *_ptr;
        size_t _count;

        public:
        friend void swap(buffer &lhs, buffer &rhs)
        {
            using std::swap;
            swap(lhs._alloc, rhs._alloc);
            swap(lhs._ptr, rhs._ptr);
            swap(lhs._count, rhs._count);
        }

        buffer() : _alloc(), _ptr(nullptr), _count(0) { }
        buffer(size_t count) : _alloc(), _ptr(nullptr), _count(count)
        {
            this->_ptr = this->_alloc.allocate(count);
        }
        buffer(Type *ptr, size_t count) : _alloc(), _ptr(nullptr), _count(count)
        {
            this->_ptr = this->_alloc.allocate(count);
            memcpy(this->_ptr, tohh(ptr), count * sizeof(Type));
        }

        buffer(buffer &&other) noexcept : _alloc(), _ptr(nullptr), _count(0) { swap(*this, other); }
        buffer &operator=(buffer &&other) noexcept { swap(*this, other); return *this; }

        buffer(const buffer &other) = delete;
        buffer &operator=(const buffer &other) = delete;

        ~buffer()
        {
            if (this->_ptr != nullptr)
                this->_alloc.deallocate(this->_ptr, this->_count);
        }

        void allocate(size_t count)
        {
            assert(count > 0);

            if (this->_ptr != nullptr)
                this->_alloc.deallocate(this->_ptr, this->_count);

            this->_ptr = this->_alloc.allocate(count);
            this->_count = count;
        }

        Type *virt_data() { return this->_ptr; }
        Type *phys_data() { return fromhh(this->_ptr); }

        const Type *virt_data() const { return this->_ptr; }
        const Type *phys_data() const { return fromhh(this->_ptr); }

        Type &at(size_t index) requires Arr
        {
            assert(this->_ptr != nullptr);
            assert(index < this->_count);
            return this->_ptr[index];
        }

        const Type &at(size_t index) const requires Arr
        {
            assert(this->_ptr != nullptr);
            assert(index < this->_count);
            return this->_ptr[index];
        }

        Type *begin() requires Arr
        {
            assert(this->_ptr != nullptr);
            return &this->at(0);
        }

        const Type *begin() const requires Arr
        {
            assert(this->_ptr != nullptr);
            return &this->at(0);
        }

        Type *end() requires Arr
        {
            assert(this->_ptr != nullptr);
            return &this->at(this->_count - 1);
        }

        const Type *end() const requires Arr
        {
            assert(this->_ptr != nullptr);
            return &this->at(this->_count - 1);
        }

        size_t size() const { return this->_count; }
        size_t size_bytes() const { return this->_count * sizeof(Type); }
    };

    using u8buffer = buffer<uint8_t, std::allocator<uint8_t>, false>;
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