// Copyright (C) 2024  ilobilo

export module system.pci;
export import :regs;

import lib;
import std;

namespace pci
{
    template<typename Type>
    concept enum_or_int = std::is_enum_v<Type> || std::integral<Type>;
} // namespace pci

export namespace pci
{
    class configio
    {
        private:
        virtual std::uint32_t read(std::uint16_t seg, std::uint8_t bus, std::uint8_t dev, std::uint8_t func, std::size_t offset, std::size_t width) = 0;
        virtual void write(std::uint16_t seg, std::uint8_t bus, std::uint8_t dev, std::uint8_t func, std::size_t offset, std::uint32_t value, std::size_t width) = 0;

        public:
        template<std::unsigned_integral Type> requires (sizeof(Type) <= sizeof(std::uint32_t))
        Type read(std::uint16_t seg, std::uint8_t bus, std::uint8_t dev, std::uint8_t func, enum_or_int auto offset)
        {
            return static_cast<Type>(read(seg, bus, dev, func, static_cast<std::size_t>(offset), sizeof(Type)));
        }

        template<std::unsigned_integral Type> requires (sizeof(Type) <= sizeof(std::uint32_t))
        void write(std::uint16_t seg, std::uint8_t bus, std::uint8_t dev, std::uint8_t func, enum_or_int auto offset, enum_or_int auto value)
        {
            write(seg, bus, dev, func, static_cast<std::size_t>(offset), static_cast<std::uint32_t>(value), sizeof(Type));
        }

        template<std::size_t N>
        auto read(std::uint16_t seg, std::uint8_t bus, std::uint8_t dev, std::uint8_t func, auto offset)
        {
            return read<lib::bits2uint_t<N>>(seg, bus, dev, func, offset);
        }

        template<std::size_t N>
        void write(std::uint16_t seg, std::uint8_t bus, std::uint8_t dev, std::uint8_t func, auto offset, auto value)
        {
            write<lib::bits2uint_t<N>>(seg, bus, dev, func, offset, value);
        }

        virtual ~configio() { }
    };

    struct bar
    {
        std::uintptr_t virt, phys;
        std::size_t size;
        bool prefetch, bits64;

        enum class type { invalid, io, mem };
        type type;

        std::uintptr_t map();
    };

    struct device;
    struct bridge;
    struct bus;

    class router
    {
        public:
        enum class model { none, root, expansion };
        enum flags
        {
            edge = (1 << 0),
            level = (1 << 1),
            high = (1 << 2),
            low = (1 << 3)
        };
        struct entry
        {
            std::uint64_t gsi;
            std::int32_t dev;
            std::int32_t func;

            std::uint8_t pin;
            std::uint8_t flags;
        };

        virtual std::unique_ptr<router> downstream(std::shared_ptr<bus> &bus) = 0;
        auto resolve(std::int32_t dev, std::uint8_t pin, std::int32_t func = -1) -> entry *;

        virtual ~router() { }

        protected:
        std::vector<entry> table;
        std::array<entry *, 4> bridge_irq;
        model mod;
    };

    struct bus
    {
        std::uint16_t seg;
        std::uint8_t id;

        std::shared_ptr<configio> io;
        std::shared_ptr<bridge> bridge;
        std::unique_ptr<router> router;

        std::vector<std::shared_ptr<device>> devices { };
        std::vector<std::shared_ptr<pci::bridge>> bridges { };

        template<typename Type>
        Type read(std::uint8_t dev, std::uint8_t func, auto offset) const
        {
            lib::ensure(static_cast<bool>(io));
            return io->read<Type>(seg, id, dev, func, offset);
        }

        template<typename Type>
        void write(std::uint8_t dev, std::uint8_t func, auto offset, auto value) const
        {
            lib::ensure(static_cast<bool>(io));
            io->write<Type>(seg, id, dev, func, offset, value);
        }

        template<std::size_t N>
        auto read(std::uint8_t dev, std::uint8_t func, auto offset) const
        {
            return read<lib::bits2uint_t<N>>(dev, func, offset);
        }

        template<std::size_t N>
        void write(std::uint8_t dev, std::uint8_t func, auto offset, auto value) const
        {
            write<lib::bits2uint_t<N>>(dev, func, offset, value);
        }
    };

    struct entity
    {
        std::uint8_t dev, func;
        std::shared_ptr<pci::bus> parent;

        bool is_pcie;
        bool is_secondary;

        entity(std::shared_ptr<pci::bus> parent, std::uint8_t dev, std::uint8_t func)
            : dev { dev }, func { func }, parent { parent } { }

        template<typename Type>
        Type read(auto offset) const
        {
            lib::ensure(static_cast<bool>(parent));
            return parent->read<Type>(dev, func, offset);
        }

        template<typename Type>
        void write(auto offset, auto value) const
        {
            lib::ensure(static_cast<bool>(parent));
            parent->write<Type>(dev, func, offset, value);
        }

        template<std::size_t N>
        auto read(auto offset) const
        {
            return read<lib::bits2uint_t<N>>(offset);
        }

        template<std::size_t N>
        void write(auto offset, auto value) const
        {
            write<lib::bits2uint_t<N>>(offset, value);
        }

        virtual std::span<bar> get_bars() = 0;
        virtual ~entity() { }
    };
    struct bridge : entity
    {
        std::uint8_t secondary_bus;
        std::uint8_t subordinate_bus;

        std::array<bar, 2> bars;

        bridge(std::shared_ptr<pci::bus> bus, std::uint8_t dev, std::uint8_t func)
            : entity { bus, dev, func } { }

        std::span<bar> get_bars() override { return bars; }
    };
    struct device : entity
    {
        std::uint16_t venid, devid;
        std::uint8_t progif, subclass, class_;
        std::array<bar, 6> bars;

        struct {
            router::entry *route;
            bool registered;
            std::size_t idx;
        } irq;

        device(std::shared_ptr<pci::bus> bus, std::uint8_t dev, std::uint8_t func)
            : entity { bus, dev, func } { }

        std::span<bar> get_bars() override { return bars; }
    };

    void addio(std::shared_ptr<configio> io, std::uint16_t seg, std::uint16_t bus);
    std::shared_ptr<configio> getio(std::uint16_t seg, std::uint8_t bus);
    void addrb(std::shared_ptr<bus> rb);

    const std::vector<std::shared_ptr<device>> &devices();

    void register_ios();
    void register_rbs();
    void init();
} // export namespace pci