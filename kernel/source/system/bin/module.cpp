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
            std::string name;
            std::string description;

            std::uintptr_t addr;
            std::vector<
                std::pair<
                    std::uintptr_t,
                    std::size_t
                >
            > memory;

            std::remove_const_t<
                decltype(::mod::declare<0>::interface)
            > interface;
            std::vector<std::string> deps;

            status status;
        };

        lib::map::flat_hash<std::string_view, entry> modules;
    } // namespace

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