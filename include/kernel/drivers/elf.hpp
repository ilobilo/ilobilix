// Copyright (C) 2022  ilobilo

#pragma once

#include <drivers/vfs.hpp>
#include <module.hpp>
#include <mm/vmm.hpp>
#include <lib/elf.h>

#include <unordered_map>
#include <string_view>
#include <optional>
#include <cstdint>
#include <vector>
#include <tuple>
#include <span>

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
        std::pair<symentry_t, uintptr_t> lookup(uintptr_t addr, uint8_t type);

        void init();
    } // namespace syms

    namespace module
    {
        struct module_t
        {
            uintptr_t addr;
            size_t size;
            bool has_drivers;
        };
        extern std::unordered_map<std::string_view, driver_t*> drivers;
        extern std::vector<module_t> modules;

        std::optional<std::vector<driver_t*>> load(uintptr_t address, size_t size);
        static inline std::optional<std::vector<driver_t*>> load(auto address, size_t size)
        {
            return load(reinterpret_cast<uintptr_t>(address), size);
        }

        std::optional<std::vector<driver_t*>> load(vfs::node_t *node);
        std::optional<std::vector<driver_t*>> load(vfs::node_t *parent, path_view_t directory);

        bool run(driver_t *drv, bool deps = true);
        void run_all(bool deps = true);

        void destroy(driver_t *entry);
        void destroy_all();

        void init();
    } // namespace module

    namespace exec
    {
        struct auxval
        {
            uint64_t at_entry;
            uint64_t at_phdr;
            uint64_t at_phent;
            uint64_t at_phnum;
        };
        std::optional<std::pair<auxval, std::string>> load(vfs::resource *res, vmm::pagemap *pagemap, uintptr_t base);
        uintptr_t prepare_stack(uintptr_t _stack, uintptr_t sp, std::span<std::string_view> argv, std::span<std::string_view> envp, auxval auxv);
    } // namespace exec
} // namespace elf