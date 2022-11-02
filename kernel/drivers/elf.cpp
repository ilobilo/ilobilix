// Copyright (C) 2022  ilobilo

#include <drivers/elf.hpp>
#include <init/kernel.hpp>
#include <lib/misc.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <smart_ptr>
#include <errno.h>
#include <cstddef>
#include <cstring>

namespace elf
{
    namespace syms
    {
        std::vector<symentry_t> symbol_table;

        symentry_t lookup(std::string_view name)
        {
            for (const auto entry : symbol_table)
            {
                if (entry.name == name)
                    return entry;
            }
            return empty_sym;
        }

        std::tuple<symentry_t, uintptr_t> lookup(uintptr_t addr, uint8_t type)
        {
            auto prev = symbol_table.front();
            for (const auto &entry : symbol_table)
            {
                if (entry.addr >= addr && entry.type == type)
                    return { prev, entry.addr - prev.addr };

                prev = entry;
            }
            return { empty_sym, 0 };
        }

        void init()
        {
            log::infoln("Initialising kernel symbol table...");

            auto kfile = reinterpret_cast<uintptr_t>(kernel_file_request.response->kernel_file->address);
            auto header = reinterpret_cast<Elf64_Ehdr*>(kfile);
            auto sections = reinterpret_cast<Elf64_Shdr*>(kfile + header->e_shoff);

            Elf64_Sym *symtab = nullptr;
            char *strtab = nullptr;
            size_t entries = 0;

            for (size_t i = 0; i < header->e_shnum; i++)
            {
                switch (sections[i].sh_type)
                {
                    case SHT_SYMTAB:
                        symtab = reinterpret_cast<Elf64_Sym*>(kfile + sections[i].sh_offset);
                        entries = sections[i].sh_size / sections[i].sh_entsize;
                        break;
                    case SHT_STRTAB:
                        strtab = reinterpret_cast<char*>(kfile + sections[i].sh_offset);
                        break;
                }
            }

            for (size_t i = 0; i < entries; i++)
            {
                auto name = std::string_view(&strtab[symtab[i].st_name]);

                // Remove aarch64 mapping symbols
                #if defined(__aarch64__)
                if (name.starts_with("$x") || name.starts_with("$d"))
                    continue;
                #endif

                symbol_table.push_back({
                    name,
                    symtab[i].st_value,
                    symtab[i].st_size,
                    uint8_t(ELF64_ST_TYPE(symtab[i].st_info))
                });
            }

            for (size_t i = 0; i < entries - 1; i++)
            {
                size_t mi = i;
                for (size_t t = i + 1; t < entries; t++)
                    if (symbol_table[t].addr < symbol_table[mi].addr)
                        mi = t;

                std::swap(symbol_table[i], symbol_table[mi]);
            }
        }
    } // namespace syms

    namespace module
    {
        std::unordered_map<std::string_view, driver_t*> drivers;
        std::vector<module_t*> modules;
        // static uint64_t base_addr = 0;
        static lock_t lock;

        static std::vector<driver_t*> get_drivers(Elf64_Ehdr *header, Elf64_Shdr *sections, char *strtable)
        {
            std::vector<driver_t*> ret;
            for (size_t i = 0; i < header->e_shnum; i++)
            {
                auto section = &sections[i];
                if (section->sh_size != 0 && section->sh_size >= sizeof(driver_t) && !strcmp(DRIVER_SECTION, strtable + section->sh_name))
                {
                    uint64_t offset = section->sh_addr;
                    while (offset < section->sh_addr + section->sh_size)
                    {
                        auto driver = reinterpret_cast<driver_t*>(offset);
                        std::string_view name(driver->name);

                        if (drivers.contains(name))
                        {
                            log::infoln("ELF: Driver with name \"{}\" already exists!", name);
                            goto next;
                        }

                        log::infoln("ELF: Found driver: \"{}\"", name);

                        if (driver->depcount > 0)
                        {
                            log::infoln(" Dependencies: ");
                            for (size_t d = 0; d < driver->depcount; d++)
                                log::infoln("  - \"{}\"", driver->deps[d]);
                        }

                        drivers[driver->name] = driver;
                        ret.push_back(driver);

                        next:
                        offset += sizeof(driver_t);
                    }
                    break;
                }
            }
            return ret;
        }

        uint64_t map(uint64_t size)
        {
            // Align to 1mib because on x86_64 OVMF aligning to 4kib causes #GPF
            // if (base_addr == 0)
            //     base_addr = align_up(tohh(pmm::mem_top), 0x40000000);

            // size = align_up(size, pmm::page_size);

            // uint64_t loadaddr = base_addr;
            // base_addr += size;

            // uint64_t paddr = pmm::alloc<uint64_t>(size / pmm::page_size);
            // vmm::kernel_pagemap->map_range(loadaddr, paddr, size, vmm::rwx);

            // return loadaddr;

            return malloc<uint64_t>(size);
        }

        void unmap(uint64_t loadaddr, uint64_t size)
        {
            // size = align_up(size, pmm::page_size);
            // if (loadaddr == base_addr - size)
            //     base_addr = loadaddr;

            // void *ptr = reinterpret_cast<void*>(vmm::kernel_pagemap->virt2phys(loadaddr));
            // pmm::free(ptr, size / pmm::page_size);
            // vmm::kernel_pagemap->unmap_range(loadaddr, size);

            free(loadaddr);
        }

        [[clang::no_sanitize("alignment")]]
        std::optional<std::vector<driver_t*>> load(uint64_t address, uint64_t size)
        {
            auto header = reinterpret_cast<Elf64_Ehdr*>(address);
            if (memcmp(header->e_ident, ELFMAG, 4))
            {
                log::errorln("ELF: Invalid magic!");
                return std::nullopt;
            }
            if (header->e_ident[EI_CLASS] != ELFCLASS64)
            {
                log::errorln("ELF: Invalid class!");
                return std::nullopt;
            }
            if (header->e_type != ET_REL)
            {
                log::errorln("ELF: Not a relocatable object!");
                return std::nullopt;
            }

            lockit(lock);
            uint64_t loadaddr = map(size);
            memcpy(reinterpret_cast<void*>(loadaddr), reinterpret_cast<void*>(address), size);
            header = reinterpret_cast<Elf64_Ehdr*>(loadaddr);

            auto sections = reinterpret_cast<Elf64_Shdr*>(loadaddr + header->e_shoff);

            for (size_t i = 0; i < header->e_shnum; i++)
            {
                auto section = &sections[i];

                if (section->sh_type == SHT_NOBITS)
                {
                    if (section->sh_size == 0)
                        continue;
                    section->sh_addr = map(section->sh_size);
                    section->sh_offset = section->sh_addr - loadaddr;
                    memset(reinterpret_cast<void*>(section->sh_addr), 0, section->sh_size);
                }
                else
                {
                    section->sh_addr = loadaddr + section->sh_offset;
                    if (section->sh_addralign && (section->sh_addr & (section->sh_addralign - 1)))
                        log::warnln("ELF: Module not aligned correctly! 0x{:X} {}", section->sh_addr, section->sh_addralign);
                }
            }

            // #if defined(__aarch64__)
            // auto aarch64_imm_adr = [](uint32_t val) -> uint32_t
            // {
            //     uint32_t low  = (val & 0x03) << 29;
            //     uint32_t high = ((val >> 2) & 0x07FFFF) << 5;
            //     return low | high;
            // };

            // auto aarch64_imm_12 = [](uint32_t val) -> uint32_t
            // {
            //     return (val & 0x0FFF) << 10;
            // };
            // #endif

            for (size_t i = 0; i < header->e_shnum; i++)
            {
                auto section = &sections[i];
                if (section->sh_type == SHT_RELA)
                {
                    auto tgtsect = &sections[section->sh_info];
                    auto entries = reinterpret_cast<Elf64_Rela*>(section->sh_addr);
                    auto symtable = reinterpret_cast<Elf64_Sym*>(sections[section->sh_link].sh_addr);
                    auto strtable = reinterpret_cast<char*>(sections[sections[section->sh_link].sh_link].sh_addr);

                    for (size_t rela = 0; rela < (section->sh_size / section->sh_entsize); rela++)
                    {
                        auto entry = &entries[rela];
                        auto sym = symtable + ELF64_R_SYM(entry->r_info);
                        auto name = strtable + sym->st_name;

                        void *loc = reinterpret_cast<void*>(tgtsect->sh_addr + entry->r_offset);
                        auto locaddr = reinterpret_cast<uintptr_t>(loc);
                        uintptr_t val = 0;

                        if (sym->st_shndx == SHN_UNDEF)
                        {
                            auto entry = syms::lookup(name);
                            if (entry == syms::empty_sym)
                            {
                                log::errorln("ELF: Could not find kernel symbol \"{}\"", name);
                                unmap(loadaddr, size);
                                return std::nullopt;
                            }
                            val = entry.addr;
                        }
                        else val = sections[sym->st_shndx].sh_addr + sym->st_value + entry->r_addend;

                        switch (ELF64_R_TYPE(entry->r_info))
                        {
                            #if defined(__x86_64__)
                            case R_X86_64_NONE:
                                break;
                            case R_X86_64_64:
                                *static_cast<uint64_t*>(loc) = val;
                                break;
                            case R_X86_64_PC64:
                                *static_cast<uint64_t*>(loc) = val - locaddr;
                                break;
                            case R_X86_64_32:
                                *static_cast<uint32_t*>(loc) = val;
                                break;
                            case R_X86_64_PC32:
                            case R_X86_64_PLT32:
                                *static_cast<uint32_t*>(loc) = val - locaddr;
                                break;
                            #elif defined(__aarch64__)
                            case R_AARCH64_NONE:
                                break;
                            case R_AARCH64_ABS64:
                                *static_cast<uint64_t*>(loc) = val;
                                break;
                            case R_AARCH64_ABS32:
                                *static_cast<uint32_t*>(loc) = val;
                                break;
                            case R_AARCH64_ABS16:
                                *static_cast<uint16_t*>(loc) = val;
                                break;
                            case R_AARCH64_PREL64:
                                *static_cast<uint64_t*>(loc) = val - locaddr;
                                break;
                            case R_AARCH64_PREL32:
                                *static_cast<uint32_t*>(loc) = val - locaddr;
                                break;
                            case R_AARCH64_PREL16:
                                *static_cast<uint16_t*>(loc) = val - locaddr;
                                break;
                            // case R_AARCH64_MOVW_UABS_G0_NC:
                            // case R_AARCH64_MOVW_UABS_G0:
                            // {
                            //     uint32_t mask = ((1 << 16) - 1);
                            //     size_t shift = 5;

                            //     uint32_t insn = *static_cast<uint32_t*>(loc);
                            //     int64_t imm = val >> 0;
                            //     *static_cast<uint64_t*>(loc) = (insn & ~(mask << shift)) | ((imm & mask) << shift);
                            //     break;
                            // }
                            // case R_AARCH64_MOVW_UABS_G1_NC:
                            // case R_AARCH64_MOVW_UABS_G1:
                            // {
                            //     uint32_t mask = ((1 << 16) - 1);
                            //     size_t shift = 5;

                            //     uint32_t insn = *static_cast<uint32_t*>(loc);
                            //     int64_t imm = val >> 16;
                            //     *static_cast<uint64_t*>(loc) = (insn & ~(mask << shift)) | ((imm & mask) << shift);
                            //     break;
                            // }
                            // case R_AARCH64_MOVW_UABS_G2_NC:
                            // case R_AARCH64_MOVW_UABS_G2:
                            // {
                            //     uint32_t mask = ((1 << 16) - 1);
                            //     size_t shift = 5;

                            //     uint32_t insn = *static_cast<uint32_t*>(loc);
                            //     int64_t imm = val >> 32;
                            //     *static_cast<uint64_t*>(loc) = (insn & ~(mask << shift)) | ((imm & mask) << shift);
                            //     break;
                            // }
                            // case R_AARCH64_MOVW_UABS_G3:
                            // {
                            //     uint32_t mask = ((1 << 16) - 1);
                            //     size_t shift = 5;

                            //     uint32_t insn = *static_cast<uint32_t*>(loc);
                            //     int64_t imm = val >> 48;
                            //     *static_cast<uint64_t*>(loc) = (insn & ~(mask << shift)) | ((imm & mask) << shift);
                            //     break;
                            // }
                            // case R_AARCH64_JUMP26:
                            // case R_AARCH64_CALL26:
                            // {
                            //     uint32_t mask = ((1 << 26) - 1);
                            //     size_t shift = 0;

                            //     uint32_t insn = *static_cast<uint32_t*>(loc);
                            //     int64_t imm = ((val - reinterpret_cast<uintptr_t>(loc)) >> 2) & (((1 << (2 + 26)) - 1) >> 2);
                            //     *static_cast<uint64_t*>(loc) = (insn & ~(mask << shift)) | ((imm & mask) << shift);
                            //     break;
                            // }
                            #endif
                            default:
                                log::errorln("ELF: Unsupported relocation {} found!", ELF64_R_TYPE(entry->r_info));
                                unmap(loadaddr, size);
                                return std::nullopt;
                        }
                    }
                }
            }

            auto mod = new module_t
            {
                .addr = loadaddr,
                .size = size,
                .has_drivers = false
            };

            auto strtable = reinterpret_cast<char*>(loadaddr + sections[header->e_shstrndx].sh_offset);
            auto drvs = get_drivers(header, sections, strtable);
            mod->has_drivers = drvs.empty() == false;

            if (mod->has_drivers == false)
                log::warnln("ELF: Could not find any drivers in the module!");

            modules.push_back(mod);
            return drvs;
        }

        std::optional<std::vector<driver_t*>> load(vfs::node_t *node)
        {
            auto size = node->res->stat.st_size;
            auto buffer = std::make_unique<uint8_t[]>(size);
            node->res->read(buffer.get(), 0, size);
            return load(buffer.get(), size);
        }

        std::optional<std::vector<driver_t*>> load(vfs::node_t *parent, path_view_t directory)
        {
            auto node = std::get<1>(vfs::path2node(parent, directory));
            if (node == nullptr)
                return std::nullopt;

            log::infoln("ELF: Loading modules from \"{}\"", directory);

            node->fs->populate(node);
            std::vector<driver_t*> ret;

            for (auto [name, child] : node->children)
            {
                if (child->type() == s_iflnk)
                    child = child->reduce(true);

                if (child->type() != s_ifreg)
                    continue;

                auto drvs = load(child);

                if (drvs.has_value())
                    ret.insert(ret.end(), drvs.value().begin(), drvs.value().end());
            }

            if (ret.empty())
            {
                log::errorln("ELF: Could not find any modules in \"{}\"", directory);
                return std::nullopt;
            }

            return ret;
        }

        bool run(driver_t *driver, bool deps)
        {
            if (driver->initialised == true)
                return true;

            log::infoln("ELF: Running driver \"{}\"", std::string_view(driver->name));
            for (size_t i = 0; i < driver->depcount; i++)
            {
                const auto &dep = driver->deps[i];
                if (!strcmp(driver->name, dep))
                    continue;

                if (drivers.contains(dep) == false)
                {
                    log::errorln("ELF: Dependency \"{}\" of driver \"{}\" not found!", dep, driver->name);
                    return false;
                }

                auto depdriver = drivers[dep];
                if (depdriver->initialised == false)
                {
                    if (deps == false)
                    {
                        log::errorln("ELF: Dependency \"{}\" of driver \"{}\" unresolved!", dep, driver->name);
                        return false;
                    }
                    run(depdriver, deps);
                }
            }

            bool ret = false;
            if (driver->init)
                ret = driver->init();
            else
                log::errorln("ELF: Driver \"{}\" does not have init() function!", driver->name);

            return driver->initialised = ret;
        }

        bool run_all(bool deps)
        {
            for (const auto [name, driver] : drivers)
                if (run(driver, deps) == false)
                    return false;

            return true;
        }

        void destroy(driver_t *driver)
        {
            if (driver->initialised == false)
                return;

            log::infoln("ELF: Destroying driver \"{}\"", driver->name);

            if (driver->fini)
                driver->fini();
            else
                log::warnln("ELF: Driver \"{}\" does not have fini() function!", driver->name);

            driver->initialised = false;
        }

        void destroy_all()
        {
            for (const auto [name, driver] : drivers)
                destroy(driver);
        }

        void init()
        {
            log::infoln("Initialising builtin drivers...");

            auto kfile = reinterpret_cast<uintptr_t>(kernel_file_request.response->kernel_file->address);
            auto header = reinterpret_cast<Elf64_Ehdr*>(kfile);
            auto sections = reinterpret_cast<Elf64_Shdr*>(kfile + header->e_shoff);
            auto strtable = reinterpret_cast<char*>(kfile + sections[header->e_shstrndx].sh_offset);

            auto drvs = get_drivers(header, sections, strtable);
            if (drvs.empty())
                log::errorln("Could not find any builtin drivers!");

            elf::module::load(nullptr, "/lib/modules/");
            elf::module::run_all();
        }
    } // namespace module

    namespace exec
    {
    } // namespace exec
} // namespace elf