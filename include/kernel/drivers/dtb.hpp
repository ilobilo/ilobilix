// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <frg/manual_box.hpp>
#include <lib/endian.hpp>
#include <string_view>
#include <utility>
#include <span>

#include <assert.h>
#include <string.h>
#include <cstddef>

namespace dtb
{
    namespace fdt
    {
        enum class tags : uint32_t
        {
            begin_node = 1,
            end_node = 2,
            prop = 3,
            nop = 4,
            end = 9
        };

        struct header
        {
            big_uint32_t magic;
            big_uint32_t totalsize;
            big_uint32_t off_dt_struct;
            big_uint32_t off_dt_strings;
            big_uint32_t off_mem_rsvmap;
            big_uint32_t version;
            big_uint32_t last_comp_version;
            big_uint32_t boot_cpuid_phys;
            big_uint32_t size_dt_strings;
            big_uint32_t size_dt_struct;
        };

        struct reserve_entry
        {
            big_uint64_t address;
            big_uint64_t size;
        };

        struct property
        {
            big_uint32_t len;
            big_uint32_t nameoff;
        };
    } // namespace fdt

    struct node;
    struct devicetree
    {
        std::byte *data;
        size_t size;

        std::byte *structs;
        std::byte *strings;

        devicetree(void *data) : data(static_cast<std::byte*>(data))
        {
            auto header = static_cast<dtb::fdt::header*>(data);
            assert(header->magic.load() == 0xD00DFEED);

            this->size = header->totalsize.load();
            this->structs = this->data + header->off_dt_struct.load();
            this->strings = this->data + header->off_dt_strings.load();
        }

        node root();
    };

    namespace detail
    {
        inline fdt::tags readtag(std::byte *&ptr)
        {
            fdt::tags tag;
            do {
                tag = static_cast<fdt::tags>(from_endian<std::endian::big>(*reinterpret_cast<uint32_t*>(ptr)));
                ptr += 4;
            } while (tag == fdt::tags::nop);
            assert(tag != fdt::tags::end);
            return tag;
        }

        inline const char *readstr(std::byte *&ptr)
        {
            auto str = reinterpret_cast<char *>(ptr);
            ptr += (strlen(str) + 4) & ~3;
            return str;
        }

        inline const char *readstr(devicetree *tree, std::byte *&ptr)
        {
            auto off = from_endian<std::endian::big>(*reinterpret_cast<uint32_t*>(ptr));
            ptr += 4;
            return reinterpret_cast<const char *>(tree->strings) + off;
        }

        inline uint32_t readlen(std::byte *&ptr)
        {
            auto len = from_endian<std::endian::big>(*reinterpret_cast<uint32_t*>(ptr));
            ptr += 4;
            return len;
        }

        inline std::span<std::byte> readprops(std::byte *&ptr, uint32_t len)
        {
            auto data = ptr;
            ptr += (len + 3) & ~3;
            return { data, len };
        }

        inline void skipprops(std::byte *&ptr)
        {
            auto len = readlen(ptr);
            ptr += 4;
            ptr += (len + 3) & ~3;
        }
    } // namespace detail

    template<typename Type>
    concept is_walker = requires (Type a, node b) {
        a.push(b);
        a.pop();
    };

    struct node
    {
        struct property_range
        {
            struct iterator
            {
                devicetree *tree;
                std::byte *ptr;

                bool operator==(const iterator &rhs) const = default;

                std::pair<std::string_view, std::span<const std::byte>> operator*()
                {
                    auto tmp = this->ptr;

                    auto tag = detail::readtag(tmp);
                    assert(tag == fdt::tags::prop);

                    auto length = detail::readlen(tmp);
                    auto name = detail::readstr(this->tree, tmp);
                    auto data = detail::readprops(tmp, length);
                    return { name, data };
                }

                iterator &operator++()
                {
                    this->ptr += 4;
                    detail::skipprops(this->ptr);
                    detail::readtag(this->ptr);
                    this->ptr -= 4;
                    return *this;
                }

                iterator &operator++(int)
                {
                    return this->operator++();
                }
            };

            devicetree *tree;
            const iterator _begin;
            const iterator _end;

            property_range(devicetree *tree, std::byte *_begin, std::byte *_end) :
                tree(tree), _begin(tree, _begin), _end(tree, _end) { }

            iterator begin() const
            {
                return this->_begin;
            }

            iterator end() const
            {
                return this->_end;
            }
        };

        private:
        devicetree *tree;
        std::byte *base;

        std::string_view _name;
        std::byte *prop_off;
        std::byte *node_off;
        frg::manual_box<property_range> _properties;

        public:
        node() : tree(nullptr), base(nullptr), _name(), prop_off(nullptr), node_off(nullptr) { }
        node(devicetree *tree, std::byte *base) : tree(tree), base(base)
        {
            std::byte *tmp = base;
            auto tag = detail::readtag(tmp);
            assert(tag == fdt::tags::begin_node);

            this->_name = detail::readstr(tmp);
            this->prop_off = tmp;

            while (true)
            {
                fdt::tags tag = detail::readtag(tmp);
                if (tag != fdt::tags::prop)
                    break;

                detail::skipprops(tmp);
            }
            this->node_off = tmp - 4;

            this->_properties.initialize(tree, this->prop_off, this->node_off);
        }

        bool operator==(const node &rhs) const
        {
            return this->tree == rhs.tree && this->base == rhs.base;
        }

        void walk(is_walker auto &&walker)
        {
            std::byte *ptr = this->node_off;
            size_t depth = 0;

            while (true)
            {
                auto tag = detail::readtag(ptr);
                switch (tag)
                {
                    case fdt::tags::begin_node:
                        depth++;
                        walker.push(node { this->tree, ptr - 4 });
                        detail::readstr(ptr);
                        break;
                    case fdt::tags::end_node:
                        walker.pop();
                        if (!depth--)
                            return;
                        break;
                    case fdt::tags::prop:
                        detail::skipprops(ptr);
                        break;
                    default:
                        assert(false, "Unknown tag");
                }
            }
        }

        template<typename Pred, typename Func>
        void subnodes(Pred pred, Func func)
        {
            struct {
                size_t depth;
                Pred &pred;
                Func &func;

                void push(node _node)
                {
                    depth++;
                    if (depth != 1)
                        return;
                    if (pred(_node))
                        func(_node);
                }
                void pop() { this->depth--; }
            } walker { 0, pred, func };
            this->walk(walker);
        }

        const property_range properties()
        {
            return *this->_properties;
        }

        const std::string_view name() const
        {
            return this->_name;
        }
    };

    extern frg::manual_box<devicetree> tree;
    void init();
} // namespace dtb