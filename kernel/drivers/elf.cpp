// Copyright (C) 2022-2024  ilobilo

#include <drivers/elf.hpp>
#include <init/kernel.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <smart_ptr>
#include <cstddef>
#include <cstring>

namespace elf
{
    namespace syms
    {
        std::vector<std::vector<symentry_t>> symbol_tables;

        symentry_t lookup(std::string_view name)
        {
            for (const auto &symbol_table : symbol_tables)
            {
                auto i = std::find_if(symbol_table.begin(), symbol_table.end(), [&name](const symentry_t &entry) {
                    return entry.name == name;
                });
                if (i != symbol_table.end())
                    return *i;
            }

            // for (const auto &symbol_table : symbol_tables)
            // {
            //     for (const auto entry : symbol_table)
            //     {
            //         if (entry.name == name)
            //             return entry;
            //     }
            // }
            return empty_sym;
        }

        std::tuple<symentry_t, uintptr_t, bool> lookup(uintptr_t addr, uint8_t type)
        {
            for (size_t i = 0; const auto &symbol_table : symbol_tables)
            {
                i++;
                if (symbol_table.empty())
                    continue;

                auto prev = symbol_table.front();
                auto end = symbol_table.back();
                if (addr < prev.addr || addr > (end.addr + end.size))
                    continue;

                for (const auto &entry : symbol_table)
                {
                    if (entry.addr >= addr && prev.addr <= addr && prev.type == type)
                        return { prev, addr - prev.addr, i > 1 };
                    prev = entry;
                }
            }
            return { empty_sym, 0, false };
        }

        static std::vector<symentry_t> add_syms(uintptr_t address, const Elf64_Half e_shnum, Elf64_Shdr *sections)
        {
            const char *strtable = nullptr;
            Elf64_Sym *symtab = nullptr;
            size_t entries = 0;

            for (size_t i = 0; i < e_shnum; i++)
            {
                switch (sections[i].sh_type)
                {
                    case SHT_SYMTAB:
                        symtab = reinterpret_cast<Elf64_Sym*>(address + sections[i].sh_offset);
                        entries = sections[i].sh_size / sections[i].sh_entsize;
                        break;
                    case SHT_STRTAB:
                        strtable = reinterpret_cast<char*>(address + sections[i].sh_offset);
                        break;
                }
            }

            std::vector<symentry_t> symbol_table;
            for (size_t i = 0; i < entries; i++)
            {
                auto name = std::string_view(&strtable[symtab[i].st_name]);
                if (symtab[i].st_shndx == SHN_UNDEF || name.empty())
                    continue;

                // Remove aarch64 mapping symbols
#if defined(__aarch64__)
                if (name.starts_with("$x") || name.starts_with("$d"))
                    continue;
#endif

                auto val = symtab[i].st_value;
                symbol_table.push_back({
                    name,
                    val < address ? val + address : val,
                    symtab[i].st_size,
                    uint8_t(ELF64_ST_TYPE(symtab[i].st_info))
                });
            }

            std::sort(symbol_table.begin(), symbol_table.end());
            return std::move(symbol_table);
        }

        void init()
        {
            log::infoln("kernel symbol table: Initialising...");

            auto kfile = reinterpret_cast<uintptr_t>(kernel_file_request.response->kernel_file->address);
            auto header = reinterpret_cast<Elf64_Ehdr*>(kfile);
            auto sections = reinterpret_cast<Elf64_Shdr*>(kfile + header->e_shoff);

            symbol_tables.push_back(add_syms(kfile, header->e_shnum, sections));
        }
    } // namespace syms

    namespace modules
    {
        std::unordered_map<std::string_view, driver_t*> drivers;
        std::vector<module_t> modules;
        static std::mutex lock;

        static std::vector<driver_t*> get_drivers(const Elf64_Half e_shnum, Elf64_Shdr *sections, char *shstrtable)
        {
            std::vector<driver_t*> ret;
            for (size_t i = 0; i < e_shnum; i++)
            {
                auto section = &sections[i];
                if (section->sh_size != 0 && section->sh_size >= sizeof(driver_t) && !strcmp(DRIVER_SECTION, shstrtable + section->sh_name))
                {
                    auto offset = section->sh_addr;
                    while (offset < section->sh_addr + section->sh_size)
                    {
                        auto driver = reinterpret_cast<driver_t*>(offset);
                        std::string_view name(driver->name);

                        if (drivers.contains(name))
                        {
                            log::infoln("ELF: Driver with name '{}' already exists", name);
                            goto next;
                        }

                        log::infoln("ELF: Found driver: '{}'", name);

                        if (driver->depcount > 0)
                        {
                            log::infoln(" Dependencies: ");
                            for (size_t d = 0; d < driver->depcount; d++)
                                log::infoln("  - '{}'", driver->deps[d]);
                        }

                        drivers[name] = driver;
                        ret.push_back(driver);

                        next:
                        offset += sizeof(driver_t);
                    }
                    break;
                }
            }
            return ret;
        }

        uintptr_t map(uintptr_t size)
        {
            auto &pmap = vmm::kernel_pagemap;

            uintptr_t vaddr = vmm::alloc_vspace(vmm::vsptypes::modules, size, pmap->get_psize());
            uintptr_t paddr = pmm::alloc<uintptr_t>(size / pmm::page_size);

            assert(pmap->map_range(vaddr, paddr, size, vmm::rwx));
            return vaddr;
        }

        void unmap(uintptr_t loadaddr, size_t size)
        {
            pmm::free(vmm::kernel_pagemap->virt2phys(loadaddr), size / pmm::page_size);
            assert(vmm::kernel_pagemap->unmap_range(loadaddr, size));
        }

        [[clang::no_sanitize("alignment")]]
        std::optional<std::vector<driver_t*>> load(uintptr_t address, size_t size)
        {
            auto unmappedhdr = reinterpret_cast<Elf64_Ehdr*>(address);
            if (memcmp(unmappedhdr->e_ident, ELFMAG, 4))
            {
                log::errorln("ELF: Invalid magic");
                return std::nullopt;
            }
            if (unmappedhdr->e_ident[EI_CLASS] != ELFCLASS64)
            {
                log::errorln("ELF: Invalid class");
                return std::nullopt;
            }
            if (unmappedhdr->e_type != ET_REL)
            {
                log::errorln("ELF: Not a relocatable object");
                return std::nullopt;
            }

            std::unique_lock guard(lock);

            auto realsize = size;
            size = align_up(size, pmm::page_size);

            auto loadaddr = map(size);
            log::infoln("ELF: Mapping module at 0x{:X}", loadaddr);

            memcpy(reinterpret_cast<void*>(loadaddr), reinterpret_cast<void*>(address), realsize);
            const auto header = *reinterpret_cast<Elf64_Ehdr*>(loadaddr);

            auto sections = reinterpret_cast<Elf64_Shdr*>(loadaddr + header.e_shoff);
            for (size_t i = 0; i < header.e_shnum; i++)
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

            for (size_t i = 0; i < header.e_shnum; i++)
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
                                log::errorln("ELF: Could not find kernel symbol '{}'", name);
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
                                log::errorln("ELF: Unsupported relocation '{}' found", ELF64_R_TYPE(entry->r_info));
                                unmap(loadaddr, size);
                                return std::nullopt;
                        }
                    }
                }
            }

            module_t mod
            {
                .addr = loadaddr,
                .size = size,
                .has_drivers = false
            };

            auto shstrtable = reinterpret_cast<char*>(loadaddr + sections[header.e_shstrndx].sh_offset);
            auto drvs = get_drivers(header.e_shnum, sections, shstrtable);
            mod.has_drivers = drvs.empty() == false;

            if (mod.has_drivers == true)
                syms::symbol_tables.push_back(syms::add_syms(loadaddr, header.e_shnum, sections));
            else
                log::warnln("ELF: Could not find any drivers in the module");

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

            log::infoln("ELF: Loading modules from '{}'", directory);

            node->fs->populate(node);
            std::vector<driver_t*> ret;

            for (auto [name, child] : node->res->children)
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
                log::errorln("ELF: Could not find any modules in '{}'", directory);
                return std::nullopt;
            }

            return ret;
        }

        bool run(driver_t *driver, bool deps)
        {
            if (driver->initialised == true || driver->failed == true)
                return true;

            std::string_view name(driver->name);

            for (size_t i = 0; i < driver->depcount; i++)
            {
                const auto &dep = driver->deps[i];
                if (name == dep)
                    continue;

                if (drivers.contains(dep) == false)
                {
                    log::errorln("ELF: Dependency '{}' of driver '{}' not found", dep, name);
                    return false;
                }

                auto depdriver = drivers[dep];
                if (depdriver->initialised == false)
                {
                    if (deps == false)
                    {
                        log::errorln("ELF: Dependency '{}' of driver '{}' unresolved", dep, name);
                        return false;
                    }
                    run(depdriver, deps);
                    if (depdriver->initialised == false)
                    {
                        log::errorln("ELF: Could not initialise dependency '{}' of driver '{}'", dep, name);
                        return false;
                    }
                }
            }

            bool ret = false;
            if (driver->init)
            {
                log::infoln("ELF: Running driver '{}'", name);
                ret = driver->init();
            }
            else log::errorln("ELF: Driver '{}' does not have init() function", name);

            return driver->failed = !(driver->initialised = ret);
        }

        void run_all(bool deps)
        {
            for (const auto [name, driver] : drivers)
                run(driver, deps);
        }

        void destroy(driver_t *driver)
        {
            if (driver->initialised == false)
                return;

            log::infoln("ELF: Destroying driver '{}'", driver->name);

            if (driver->fini)
                driver->fini();
            else
                log::warnln("ELF: Driver '{}' does not have fini() function", driver->name);

            driver->initialised = false;
        }

        void destroy_all()
        {
            for (const auto [name, driver] : drivers)
                destroy(driver);
        }

        void init()
        {
            log::infoln("ELF: Searching for builtin drivers...");

            auto kfile = reinterpret_cast<uintptr_t>(kernel_file_request.response->kernel_file->address);
            auto header = reinterpret_cast<Elf64_Ehdr*>(kfile);
            auto sections = reinterpret_cast<Elf64_Shdr*>(kfile + header->e_shoff);
            auto shstrtable = reinterpret_cast<char*>(kfile + sections[header->e_shstrndx].sh_offset);

            auto drvs = get_drivers(header->e_shnum, sections, shstrtable);
            if (drvs.empty())
                log::errorln("ELF: Could not find any builtin drivers");
        }
    } // namespace modules

    namespace exec
    {
        std::optional<std::pair<auxval, std::string>> load(vfs::resource *res, vmm::pagemap *pagemap, uintptr_t base)
        {
            auxval auxv { 0, 0, 0, 0 };
            std::string ld_path;

            Elf64_Ehdr header;
            if (res->read(&header, 0, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr))
                return std::nullopt;

            if (memcmp(header.e_ident, ELFMAG, 4))
                return std::nullopt;

            if (header.e_ident[EI_CLASS] != ELFCLASS64 || header.e_ident[EI_DATA] != ELFDATA2LSB || header.e_ident[EI_OSABI] != ELFOSABI_SYSV || header.e_machine != EM_X86_64)
                return std::nullopt;

            for (size_t i = 0; i < header.e_phnum; i++)
            {
                Elf64_Phdr phdr;
                if (res->read(&phdr, header.e_phoff + i * header.e_phentsize, sizeof(phdr)) != sizeof(phdr))
                    return std::nullopt;

                switch (phdr.p_type)
                {
                    case PT_LOAD:
                    {
                        size_t flags = vmm::mmap::prot_read;
                        if (phdr.p_flags & PF_W)
                            flags |= vmm::mmap::prot_write;
                        if (phdr.p_flags & PF_X)
                            flags |= vmm::mmap::prot_exec;

                        size_t misalign = phdr.p_vaddr & (pmm::page_size - 1);
                        size_t pages = div_roundup(phdr.p_memsz + misalign, pmm::page_size);

                        auto paddr = pmm::alloc<uintptr_t>(pages);
                        if (!pagemap->mmap_range(phdr.p_vaddr + base, paddr, pages * pmm::page_size, flags, vmm::mmap::map_anonymous))
                        {
                            pmm::free(paddr, pages);
                            return std::nullopt;
                        }

                        if (res->read(reinterpret_cast<void*>(tohh(paddr + misalign)), phdr.p_offset, phdr.p_filesz) != ssize_t(phdr.p_filesz))
                            return std::nullopt;

                        // uintptr_t vstart = align_down(phdr.p_vaddr, pmm::page_size);
                        // uintptr_t vend = align_up(phdr.p_vaddr + phdr.p_memsz, pmm::page_size);
                        // uintptr_t vfend = align_up(phdr.p_vaddr + phdr.p_filesz, pmm::page_size);

                        // size_t misalign = phdr.p_vaddr - vstart;

                        // if (pagemap->mmap(vstart + base, phdr.p_filesz + misalign, flags, vmm::mmap::map_private | vmm::mmap::map_fixed, res, phdr.p_offset + misalign) == vmm::mmap::map_failed)
                        //     return std::nullopt;

                        // if (vend > vfend)
                        //     if (pagemap->mmap(vfend, vend - vfend, flags, vmm::mmap::map_private | vmm::mmap::map_fixed | vmm::mmap::map_anonymous, NULL, 0) == vmm::mmap::map_failed)
                        //         return std::nullopt;

                        break;
                    }
                    case PT_PHDR:
                        auxv.at_phdr = phdr.p_vaddr + base;
                        break;
                    case PT_INTERP:
                    {
                        std::unique_ptr<char[]> ptr(new char[phdr.p_filesz + 1]);

                        if (res->read(ptr.get(), phdr.p_offset, phdr.p_filesz) != ssize_t(phdr.p_filesz))
                            return std::nullopt;

                        ld_path = ptr.get();
                        break;
                    }
                }
            }

            auxv.at_entry = header.e_entry + base;
            auxv.at_phent = header.e_phentsize;
            auxv.at_phnum = header.e_phnum;

            return std::make_pair(auxv, ld_path);
        }

        uintptr_t prepare_stack(uintptr_t _stack, uintptr_t sp, std::span<std::string_view> argv, std::span<std::string_view> envp, auxval auxv)
        {
            auto stack = reinterpret_cast<uintptr_t*>(_stack);

            for (const auto &env : envp)
            {
                stack = reinterpret_cast<uintptr_t*>(reinterpret_cast<char*>(stack) - env.length() - 1);
                memcpy(stack, env.data(), env.length());
            }

            for (const auto &arg : argv)
            {
                stack = reinterpret_cast<uintptr_t*>(reinterpret_cast<char*>(stack) - arg.length() - 1);
                memcpy(stack, arg.data(), arg.length());
            }

            stack = reinterpret_cast<uintptr_t*>(align_down(reinterpret_cast<uintptr_t>(stack), 16));
            if ((argv.size() + envp.size() + 1) & 1)
                stack--;

            *(--stack) = 0; *(--stack) = 0;
            stack -= 2; stack[0] = AT_ENTRY, stack[1] = auxv.at_entry;
            stack -= 2; stack[0] = AT_PHDR,  stack[1] = auxv.at_phdr;
            stack -= 2; stack[0] = AT_PHENT, stack[1] = auxv.at_phent;
            stack -= 2; stack[0] = AT_PHNUM, stack[1] = auxv.at_phnum;

            uintptr_t old_sp = sp;

            *(--stack) = 0;
            stack -= envp.size();
            for (size_t i = 0; const auto &env : envp)
            {
                old_sp -= env.length() + 1;
                stack[i++] = old_sp;
            }

            *(--stack) = 0;
            stack -= argv.size();
            for (size_t i = 0; const auto &arg : argv)
            {
                old_sp -= arg.length() + 1;
                stack[i++] = old_sp;
            }

            *(--stack) = argv.size();
            return sp - (_stack - reinterpret_cast<uintptr_t>(stack));
        }
    } // namespace exec
} // namespace elf