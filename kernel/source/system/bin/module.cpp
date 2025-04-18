// Copyright (C) 2024-2025  ilobilo

module;

#include <elf.h>

module system.bin.elf;

import system.memory;
import :mod;
import lib;
import std;

namespace bin::elf::mod
{
    namespace
    {
        enum class status { };
        struct entry
        {
            bool internal;

            std::uintptr_t addr;
            std::vector<
                std::pair<
                    std::uintptr_t,
                    std::size_t
                >
            > memory;

            ::mod::declare<0> *header;
            std::vector<std::string_view> deps;

            status status;
        };

        lib::map::flat_hash<std::string_view, entry> modules;

        void log_entry(const auto &entry)
        {
            auto ptr = entry.header;
            log::info("elf: internal module: {}", ptr->name);
            log::info("elf: - description: {}", ptr->description);

            std::visit(
                lib::overloaded {
                    [&](const ::mod::generic &) { log::info("elf: - type: generic"); },
                    [&](const ::mod::pci &) { log::info("elf: - type: pci"); },
                    [&](const ::mod::acpi &) { log::info("elf: - type: acpi"); }
                }, ptr->interface
            );

            const auto ndeps = entry.deps.size();
            log::info("elf: - dependencies: {}", ndeps);

            if (ndeps != 0)
                log::print(log::level::info, "elf: -  ");
            for (std::size_t i = 0; i < ndeps; i++)
            {
                auto mod = entry.deps[i];
                log::print("{}{}", mod, i == ndeps - 1 ? "" : ", ");
            }
            if (ndeps != 0)
                log::println();
        }
    } // namespace

    extern "C" char __section_modules_start[];
    extern "C" char __section_modules_end[];

    void load_internal()
    {
        using base_type = ::mod::declare<0>;
        constexpr std::size_t base_size = sizeof(base_type);

        const auto start = reinterpret_cast<std::uintptr_t>(__section_modules_start);
        const auto end = reinterpret_cast<std::uintptr_t>(__section_modules_end);

        auto current = start;
        while (current < end)
        {
            const auto magic = *reinterpret_cast<std::uint64_t *>(current);
            if (magic != base_type::header_magic)
            {
                current++;
                continue;
            }

            const auto ptr = reinterpret_cast<base_type *>(current);
            const auto ndeps = ptr->dependencies.ndeps;
            const auto deps = reinterpret_cast<const char *const *>(ptr->dependencies.list);

            auto &entry = modules[ptr->name];
            entry.internal = true;
            entry.header = ptr;

            for (std::size_t i = 0; i < ndeps; i++)
                entry.deps.push_back(deps[i]);

            log_entry(entry);

            current += base_size + ndeps * sizeof(const char *);
        }
    }

    bool load(std::uintptr_t addr, std::size_t size)
    {
        decltype(entry::memory) memory;
        auto map = [&memory](std::size_t size, auto flags) {
            auto &pmap = vmm::kernel_pagemap;

            const auto vaddr = vmm::alloc_vpages(vmm::space_type::modules, lib::div_roundup(size, pmm::page_size));
            for (std::size_t i = 0; i < size; i += pmm::page_size)
            {
                const auto paddr = pmm::alloc<std::uintptr_t>(1, true);
                if (!pmap->map(vaddr + i, paddr, pmm::page_size, flags))
                    lib::panic("could not map memory for a module");
                memory.emplace_back(paddr, pmm::page_size);
            }
            return vaddr;
        };

        auto ehdr = reinterpret_cast<Elf64_Ehdr *>(addr);
        if (std::memcmp(ehdr->e_ident, ELFMAG, SELFMAG))
        {
            log::error("elf: module: invalid magic");
            return false;
        }
        if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
        {
            log::error("elf: module: invalid class");
            return false;
        }
        if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
        {
            log::error("elf: module: invalid data type");
            return false;
        }
        if (ehdr->e_type != ET_REL)
        {
            log::error("elf: module: not a relocatable object");
            return false;
        }

        auto load_at = map(size, vmm::flag::rw);
        log::debug("elf: module: loading at 0x{:X}", load_at);
        std::memcpy(reinterpret_cast<void *>(load_at), reinterpret_cast<void *>(addr), size);

        ehdr = reinterpret_cast<Elf64_Ehdr *>(load_at);
        auto phdrs = reinterpret_cast<Elf64_Phdr *>(load_at + ehdr->e_phoff);
        for (std::size_t i = 0; i < ehdr->e_phnum; i++)
        {
            auto &phdr = phdrs[i];
            switch (phdr.p_type)
            {
                case PT_LOAD:
                {
                    auto flags = vmm::flag::none;
                    if (phdr.p_flags & PF_R)
                        flags |= vmm::flag::read;
                    if (phdr.p_flags & PF_W)
                        flags |= vmm::flag::write;
                    if (phdr.p_flags & PF_X)
                        flags |= vmm::flag::exec;

                    phdr.p_vaddr = map(phdr.p_memsz, flags);
                    break;
                }
                case PT_DYNAMIC:
                {
                    auto dyn = reinterpret_cast<Elf64_Dyn *>(load_at + phdr.p_offset);
                    const auto end = reinterpret_cast<Elf64_Dyn *>(load_at + phdr.p_offset + phdr.p_filesz);
                    for (; dyn < end; dyn++)
                    {
                        // switch (dyn.d_tag)
                        // {
                        //     case :
                        // }
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
} // namespace bin::elf::mod