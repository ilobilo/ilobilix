// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <fmt/format.h>
#include <cwalk.h>
#include <vector>
#include <string>

struct path_segment
{
    std::string_view segment;
    cwk_segment_type type;
    bool is_last;
    cwk_segment seg;
};

class path_view_t
{
    private:
    std::string_view _str;

    public:
    static constexpr char preferred_separator = '/';

    path_view_t() : _str("") { }

    path_view_t(const path_view_t &other) : _str(other._str) { }
    path_view_t(path_view_t &&other) : _str(std::move(other._str)) { }

    template<typename Type>
    path_view_t(const Type &source) : _str(source) { }

    // template<typename Type>
    // path_view_t(Type &&source) : _str(std::move(source)) { }

    ~path_view_t() = default;

    path_view_t &operator=(const path_view_t &p)
    {
        this->_str = p._str;
        return *this;
    }

    path_view_t &operator=(path_view_t &&p)
    {
        this->_str = std::move(p._str);
        return *this;
    }

    template<typename Type>
    path_view_t &operator=(const Type &source)
    {
        return this->assign(source);
    }

    template<typename Type>
    path_view_t &operator=(Type &&source)
    {
        return this->assign(std::move(source));
    }

    template<typename Type>
    path_view_t &assign(const Type &source)
    {
        this->_str = source;
        return *this;
    }

    template<typename Type>
    path_view_t &assign(Type &&source)
    {
        this->_str = std::move(source);
        return *this;
    }

    void swap(path_view_t &other)
    {
        using std::swap;
        this->_str.swap(other._str);
    }

    const char *data() const
    {
        return this->_str.data();
    }

    size_t size() const
    {
        return this->_str.size();
    }

    size_t length() const
    {
        return this->_str.length();
    }

    int compare(const path_view_t &p) const
    {
        return this->_str.compare(p._str);
    }

    template<typename Type>
    int compare(const Type &source) const
    {
        return this->_str.compare(source);
    }

    path_view_t basename() const
    {
        const char *buffer = nullptr;
        size_t length = 0;
        cwk_path_get_basename(this->_str.data(), &buffer, &length);

        return std::string_view(buffer, length);
    }

    path_view_t dirname() const
    {
        size_t length = 0;
        cwk_path_get_dirname(this->_str.data(), &length);

        return std::string_view(this->_str.data(), length);
    }

    path_view_t root() const
    {
        size_t length = 0;
        cwk_path_get_root(this->_str.data(), &length);
        return std::string_view(this->_str.data(), length);
    }

    bool is_absolute() const
    {
        return cwk_path_is_absolute(this->_str.data());
    }

    bool is_relative() const
    {
        return cwk_path_is_relative(this->_str.data());
    }

    path_view_t get_relative(const path_view_t &base) const
    {
        size_t length = cwk_path_get_relative(base.data(), this->_str.data(), nullptr, 0) + 1;
        auto buffer = new char[length];
        cwk_path_get_relative(base.data(), this->_str.data(), buffer, length);

        return buffer;
    }

    path_view_t get_absolute(const path_view_t &base) const
    {
        size_t length = cwk_path_get_absolute(base.data(), this->_str.data(), nullptr, 0) + 1;
        auto buffer = new char[length];
        cwk_path_get_absolute(base.data(), this->_str.data(), buffer, length);

        return buffer;
    }

    path_view_t extension() const
    {
        if (_str.empty() || _str.back() == '/')
            return "";

        const char *extension = nullptr;
        size_t length = 0;
        cwk_path_get_extension(this->_str.data(), &extension, &length);

        return std::string_view(extension, length);
    }

    bool has_extension() const
    {
        return cwk_path_has_extension(this->_str.data());
    }

    std::vector<path_segment> segments()
    {
        std::vector<path_segment> ret;

        cwk_segment seg;
        cwk_path_get_first_segment(this->_str.data(), &seg);

        do {
            ret.push_back({
                std::string_view(seg.begin, seg.size),
                cwk_path_get_segment_type(&seg),
                false,
                seg
            });
        } while (cwk_path_get_next_segment(&seg));

        ret.back().is_last = true;

        return ret;
    }

    [[nodiscard]] bool empty() const
    {
        return this->_str.empty();
    }

    char *begin() const
    {
        return const_cast<char*>(this->_str.cbegin());
    }

    char *end() const
    {
        return const_cast<char*>(this->_str.cend());
    }

    friend void swap(path_view_t &lhs, path_view_t &rhs)
    {
        lhs.swap(rhs);
    }

    friend bool operator==(const path_view_t &lhs, const path_view_t &rhs)
    {
        return lhs.compare(rhs) == 0;
    }

    friend auto operator<=>(const path_view_t &lhs, const path_view_t &rhs)
    {
        return lhs.compare(rhs) <=> 0;
    }
};

class path_t
{
    private:
    std::string _str;

    public:
    static constexpr char preferred_separator = '/';

    path_t() : _str("") { }

    path_t(const path_t &other) : _str(other._str)
    {
        this->normalise();
    }

    path_t(path_t &&other) : _str(std::move(other._str))
    {
        this->normalise();
    }

    path_t(const path_view_t &other) : _str(other.data(), other.length())
    {
        this->normalise();
    }

    path_t(path_view_t &&other) : _str(std::move(other.data()), std::move(other.length()))
    {
        this->normalise();
    }

    template<typename Type>
    path_t(const Type &source) : _str(source)
    {
        this->normalise();
    }

    ~path_t() = default;

    path_t &operator=(const path_t &p)
    {
        this->_str = p._str;
        return *this;
    }

    path_t &operator=(path_t &&p)
    {
        this->_str = std::move(p._str);
        return *this;
    }

    template<typename Type>
    path_t &operator=(const Type &source)
    {
        return this->assign(source);
    }

    template<typename Type>
    path_t &operator=(Type &&source)
    {
        return this->assign(std::move(source));
    }

    template<typename Type>
    path_t &assign(const Type &source)
    {
        this->_str = source;
        return *this;
    }

    template<typename Type>
    path_t &assign(Type &&source)
    {
        this->_str = std::move(source);
        return *this;
    }

    path_t &operator/=(const path_t &p)
    {
        if (this->_str.back() != preferred_separator)
            this->_str += preferred_separator;

        this->_str += p._str;
        return *this;
    }

    template<typename Type>
    path_t &operator/=(const Type &source)
    {
        return this->append(source);
    }

    template<typename Type>
    path_t &append(const Type &source)
    {
        if (this->_str.back() != preferred_separator)
            this->_str += preferred_separator;

        this->_str += source;
        return *this;
    }

    path_t &operator+=(const path_t &p)
    {
        this->_str += p._str;
        return *this;
    }

    template<typename Type>
    path_t &operator+=(const Type &source)
    {
        this->_str += source;
        return *this;
    }

    void clear()
    {
        this->_str.clear();
    }

    void swap(path_t &other)
    {
        using std::swap;
        this->_str.swap(other._str);
    }

    const char *c_str() const
    {
        return this->_str.c_str();
    }

    operator std::string() const
    {
        return this->_str;
    }

    operator std::string_view() const
    {
        return std::string_view { this->_str };
    }

    operator path_view_t() const
    {
        return path_view_t(this->operator std::basic_string_view<char, std::char_traits<char>>());
    }

    path_view_t view() const
    {
        return path_view_t(this->_str);
    }

    size_t size() const
    {
        return this->_str.size();
    }

    size_t length() const
    {
        return this->_str.length();
    }

    int compare(const path_t &p) const
    {
        return this->_str.compare(p._str);
    }

    template<typename Type>
    int compare(const Type &source) const
    {
        return this->_str.compare(source);
    }

    path_t basename() const
    {
        return this->view().basename();
    }

    path_t &change_basename(const path_t &newbasename = path_t())
    {
        size_t length = cwk_path_change_basename(this->_str.c_str(), newbasename._str.c_str(), nullptr, 0) + 1;
        auto buffer = new char[length];
        cwk_path_change_basename(this->_str.c_str(), newbasename._str.c_str(), buffer, length);

        this->_str = buffer;
        return *this;
    }

    path_view_t dirname() const
    {
        return this->view().dirname();
    }

    path_view_t root() const
    {
        return this->view().root();
    }

    path_t &change_root(const path_t &newroot = path_t())
    {
        size_t length = cwk_path_change_root(this->_str.c_str(), newroot._str.c_str(), nullptr, 0) + 1;
        auto buffer = new char[length];
        cwk_path_change_root(this->_str.c_str(), newroot._str.c_str(), buffer, length);

        this->_str = buffer;
        return *this;
    }

    bool is_absolute() const
    {
        return this->view().is_absolute();
    }

    bool is_relative() const
    {
        return this->view().is_relative();
    }

    path_t &normalise()
    {
        size_t length = cwk_path_normalize(this->_str.c_str(), nullptr, 0) + 1;
        auto buffer = new char[length];
        cwk_path_normalize(this->_str.c_str(), buffer, length);

        this->_str = buffer;
        return *this;
    }

    path_t get_relative(const path_t &base) const
    {
        return this->view().get_relative(base);
    }

    path_t get_absolute(const path_t &base) const
    {
        return this->view().get_absolute(base);
    }

    path_t extension() const
    {
        return this->view().extension();
    }

    bool has_extension() const
    {
        return this->view().has_extension();
    }

    path_t &change_extension(const path_t &newext = path_t())
    {
        size_t newsize = cwk_path_change_extension(this->_str.c_str(), newext._str.c_str(), nullptr, 0) + 1;
        auto buffer = new char[newsize + 1];
        cwk_path_change_extension(this->_str.c_str(), newext._str.c_str(), buffer, newsize + 1);
        this->_str = buffer;

        return *this;
    }

    std::vector<path_segment> segments()
    {
        return this->view().segments();
    }

    [[nodiscard]] bool empty() const
    {
        return this->_str.empty();
    }

    decltype(auto) begin() const
    {
        return this->_str.begin();
    }

    decltype(auto) end() const
    {
        return this->_str.end();
    }

    friend void swap(path_t &lhs, path_t &rhs)
    {
        lhs.swap(rhs);
    }

    friend bool operator==(const path_t &lhs, const path_t &rhs)
    {
        return lhs.compare(rhs) == 0;
    }

    friend auto operator<=>(const path_t &lhs, const path_t &rhs)
    {
        return lhs.compare(rhs) <=> 0;
    }
};

template<>
struct fmt::formatter<path_view_t> : formatter<std::string_view>
{
    template<typename FormatContext>
    auto format(path_view_t path, FormatContext &ctx) const
    {
        return formatter<std::string_view>::format(path.data(), ctx);
    }
};

template<>
struct fmt::formatter<path_t> : formatter<std::string>
{
    template<typename FormatContext>
    auto format(path_t path, FormatContext &ctx) const
    {
        return formatter<std::string>::format(path.c_str(), ctx);
    }
};