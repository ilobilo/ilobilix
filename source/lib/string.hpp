// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/alloc.hpp>
#include <utility>
#include <cstddef>

unsigned constexpr hash(const char *input)
{
    return *input ? static_cast<unsigned int>(*input) + 33 * hash(input + 1) : 5381;
}

size_t strlen(const char *str);

char *strcpy(char *destination, const char *source);
char *strncpy(char *destination, const char *source, size_t n);

int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);

void strrev(unsigned char *str);

long strtol(const char *nPtr, char **endPtr, int base);
long long int strtoll(const char *nPtr, char **endPtr, int base);
unsigned long strtoul(const char *nPtr, char **endPtr, int base);
unsigned long long int strtoull(const char *nPtr, char **endPtr, int base);

char *strstr(const char *str, const char *substr);

int isspace(char c);

int isdigit(int c);
int isalpha(int c);
int islower(int c);

char tolower(char c);
char toupper(char c);

int itoa(int num, unsigned char *str, int len, int base);
int atoi(const char *str);

bool isempty(char *str);
static inline bool isempty(const char *str)
{
    return isempty(const_cast<char*>(str));
}

int tonum(char c);

extern "C" void *memcpy(void *dest, const void *src, size_t len);
extern "C" int memcmp(const void *ptr1, const void *ptr2, size_t len);
extern "C" void *memset(void *dest, int ch, size_t n);
extern "C" void *memmove(void *dest, const void *src, size_t len);

// Following code is modified version of: https://github.com/managarm/frigg/blob/master/include/frg/string.hpp

template<typename Char>
class basic_string_view
{
    private:
    const Char *_pointer;
    size_t _length;

    public:
    basic_string_view() : _pointer(nullptr), _length(0) { }
    basic_string_view(const Char *cs) : _pointer(cs), _length(0)
    {
        while(cs[this->_length]) this->_length++;
    }

    basic_string_view(const Char *s, size_t length) : _pointer(s), _length(length) { }

    const Char *data() const
    {
        return _pointer;
    }

    const Char &operator[](size_t index) const
    {
        return _pointer[index];
    }

    size_t size() const
    {
        return this->_length;
    }

    bool operator==(basic_string_view other) const
    {
        if(this->_length != other._length) return false;
        for(size_t i = 0; i < this->_length; i++)
        {
            if(_pointer[i] != other._pointer[i]) return false;
        }
        return true;
    }
    bool operator!=(basic_string_view other) const
    {
        return !(*this == other);
    }

    size_t find_first(Char c, size_t start_from = 0)
    {
        for(size_t i = start_from; i < this->_length; i++)
        {
            if(_pointer[i] == c) return i;
        }
        return size_t(-1);
    }

    size_t find_last(Char c)
    {
        for(size_t i = this->_length; i > 0; i--)
        {
            if(_pointer[i - 1] == c) return i - 1;
        }

        return size_t(-1);
    }

    basic_string_view sub_string(size_t from, size_t size) const
    {
        if (from + size > this->_length) from = this->_length - size;
        return basic_string_view(_pointer + from, size);
    }

    template<typename type>
    type to_number()
    {
        type value = 0;
        for(size_t i = 0; i < this->_length; i++)
        {
            if(!(_pointer[i] >= '0' && _pointer[i] <= '9')) return 0;
            value = value * 10 + (_pointer[i] - '0');
        }
        return value;
    }
};

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;

namespace std
{
    using string_view = ::string_view;
    using wstring_view = ::wstring_view;
    using u16string_view = ::u16string_view;
    using u32string_view = ::u32string_view;
} // namespace std

template<typename Char>
class basic_string
{
    private:
    Char *_buffer = nullptr;
    size_t _length = 0;
    size_t _cap = 0;

    public:
    friend void swap(basic_string &a, basic_string &b)
    {
        using std::swap;
        swap(a._buffer, b._buffer);
        swap(a._length, b._length);
        swap(a._cap, b._cap);
    }

    basic_string() : _buffer(nullptr), _length(0), _cap(0) { }
    basic_string(const Char *c_string)
    {
        this->_length = this->_cap = strlen(c_string);
        _buffer = new Char[this->_length + 1];
        memcpy(_buffer, c_string, sizeof(Char) * this->_length);
        _buffer[this->_length] = 0;
    }

    basic_string(const Char *buffer, size_t size) : _length(size), _cap(size)
    {
        _buffer = new Char[this->_length + 1];
        memcpy(_buffer, buffer, sizeof(Char) * this->_length);
        _buffer[this->_length] = 0;
    }

    explicit basic_string(const basic_string_view<Char> &view) : _length(view.size()), _cap(view.size())
    {
        _buffer = new Char[this->_length + 1];
        memcpy(_buffer, view.data(), sizeof(Char) * this->_length + 1);
        _buffer[this->_length] = 0;
    }

    basic_string(size_t size, Char c = 0) : _length(size), _cap(size)
    {
        _buffer = new Char[this->_length + 1];
        for(size_t i = 0; i < size; i++) _buffer[i] = c;
        _buffer[this->_length] = 0;
    }

    basic_string(const basic_string &other) : _length(other._length), _cap(other._length)
    {
        _buffer = new Char[this->_length + 1];
        memcpy(_buffer, other._buffer, sizeof(Char) * this->_length);
        _buffer[this->_length] = 0;
    }

    ~basic_string()
    {
        if(_buffer) delete[] _buffer;
    }

    basic_string &operator=(const Char *c_string)
    {
        size_t new_length = strlen(c_string);
        if (new_length > this->_cap)
        {
            delete[] _buffer;
            _buffer = new Char[new_length + 1];
            this->_cap = new_length;
        }
        memcpy(_buffer, c_string, sizeof(Char) * new_length);
        _buffer[new_length] = 0;
        this->_length = new_length;
        return *this;
    }

    basic_string &operator=(basic_string other)
    {
        swap(*this, other);
        return *this;
    }

    void resize(size_t new_length)
    {
        if (new_length < _length) _length = new_length;

        _buffer = realloc<Char*>(_buffer, sizeof(Char) * new_length + 1);
        _buffer[new_length] = 0;

        this->_cap = new_length;
    }

    basic_string operator+(const basic_string_view<Char> &other)
    {
        size_t new_length = this->_length + other.size();
        Char *new_buffer = new Char[new_length + 1];
        memcpy(new_buffer, _buffer, sizeof(Char) * this->_length);
        memcpy(new_buffer + this->_length, other.data(), sizeof(Char) * other.size());
        new_buffer[new_length] = 0;

        return new_buffer;
    }

    basic_string operator+(Char c)
    {
        size_t new_length = this->_length + 1;
        Char *new_buffer = new Char[new_length + 1];
        memcpy(new_buffer, _buffer, sizeof(Char) * this->_length);
        new_buffer[this->_length] = c;
        new_buffer[new_length] = 0;

        return new_buffer;
    }

    basic_string &operator+=(const basic_string_view<Char> &other)
    {
        size_t new_length = this->_length + other.size();
        if (new_length > this->_cap)
        {
            _buffer = realloc<Char*>(_buffer, new_length);
            this->_cap = new_length;
        }

        memcpy(_buffer + this->_length, other.data(), sizeof(Char) * other.size());
        _buffer[new_length] = 0;
        this->_length = new_length;

        return *this;
    }

    basic_string &operator+=(Char c)
    {
        if (++this->_length > this->_cap)
        {
            _buffer = realloc<Char*>(_buffer, this->_length);
            this->_cap = this->_length;
        }

        _buffer[this->_length - 1] = c;
        _buffer[this->_length] = 0;

        return *this;
    }

    void detach()
    {
        _buffer = nullptr;
        this->_length = 0;
        this->_cap = 0;
    }

    Char *data()
    {
        return _buffer;
    }
    const Char *c_str() const
    {
        return _buffer;
    }

    Char &operator[](size_t index)
    {
        return _buffer[index];
    }
    const Char &operator[](size_t index) const
    {
        return _buffer[index];
    }

    size_t length() const
    {
        return this->_length;
    }

    size_t capacity() const
    {
        return this->_cap;
    }

    bool empty() const
    {
        return this->_length == 0;
    }

    Char *begin()
    {
        return _buffer;
    }
    const Char *begin() const
    {
        return _buffer;
    }

    Char *end()
    {
        return _buffer + this->_length;
    }
    const Char *end() const
    {
        return _buffer + this->_length;
    }

    int compare(const basic_string<Char> &other) const
    {
        if(this->_length != other.size()) return this->_length < other.size() ? -1 : 1;
        for(size_t i = 0; i < this->_length; i++)
        {
            if(_buffer[i] != other[i]) return _buffer[i] < other[i] ? -1 : 1;
        }
        return true;
    }

    int compare(const char *other) const
    {
        if(this->_length != strlen(other)) return this->_length < strlen(other) ? -1 : 1;
        for(size_t i = 0; i < this->_length; i++)
        {
            if(_buffer[i] != other[i]) return _buffer[i] < other[i] ? -1 : 1;
        }
        return true;
    }

    bool operator==(const basic_string<Char> &other) const
    {
        return compare(other) == 0;
    }

    bool operator==(const char *rhs) const
    {
        return compare(rhs) == 0;
    }

    bool operator!=(const basic_string_view<Char> &other) const
    {
        return !(*this == other);
    }

    operator basic_string_view<Char>() const
    {
        return basic_string_view<Char>(_buffer, this->_length);
    }

    basic_string_view<Char> sub_view() const
    {
        return basic_string_view<Char>(_buffer, this->_length);
    }
    basic_string_view<Char> sub_view(size_t start) const
    {
        return basic_string_view<Char>(_buffer + start, this->_length - start);
    }
    basic_string_view<Char> sub_view(size_t start, size_t length) const
    {
        return basic_string_view<Char>(_buffer + start, (start + length) > this->_length ? this->_length - start : length);
    }
};

using string = basic_string<char>;
using wstring = basic_string<wchar_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;

namespace std
{
    using string = ::string;
    using wstring = ::wstring;
    using u16string = ::u16string;
    using u32string = ::u32string;
} // namespace std