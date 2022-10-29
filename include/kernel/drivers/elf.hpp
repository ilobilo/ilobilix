// Copyright (C) 2022  ilobilo

#pragma once

#include <modules/module.hpp>
#include <drivers/vfs.hpp>
#include <unordered_map>
#include <string_view>
#include <lib/elf.h>
#include <cstdint>
#include <vector>
#include <tuple>

namespace elf
{
    namespace syms
    {
        struct symentry_t
        {
            std::string_view name;
            uintptr_t addr;
            size_t size;
            uint8_t type;

            constexpr bool operator==(const symentry_t &) const = default;
        };
        static constexpr symentry_t empty_sym = { "<unknown>", UINTPTR_MAX, 0, STT_NOTYPE };

        extern std::vector<symentry_t> symbol_table;

        symentry_t lookup(std::string_view name);
        std::tuple<symentry_t, uintptr_t> lookup(uintptr_t addr, uint8_t type);

        void init();
    } // namespace syms

    namespace module
    {
        struct module_t
        {
            uint64_t addr;
            uint64_t size;
            bool has_drivers;
        };
        extern std::unordered_map<std::string_view, driver_t*> drivers;
        extern std::vector<module_t*> modules;

        std::optional<std::vector<driver_t*>> load(uint64_t address, uint64_t size);
        static inline std::optional<std::vector<driver_t*>> load(auto address, uint64_t size)
        {
            return load(reinterpret_cast<uint64_t>(address), size);
        }

        std::optional<std::vector<driver_t*>> load(vfs::node_t *node);
        std::optional<std::vector<driver_t*>> load(vfs::node_t *parent, path_view_t directory);

        bool run(driver_t *drv, bool deps = true);
        bool run_all(bool deps = true);

        void destroy(driver_t *entry);
        void destroy_all();

        void init();
    } // namespace module

    namespace exec
    {
    } // namespace exec
} // namespace elf