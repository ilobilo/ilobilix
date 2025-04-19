// Copyright (C) 2024-2025  ilobilo

module;

#include <elf.h>

module system.bin.elf;

import system.memory;
import system.vfs;
import magic_enum;
import lib;
import std;

namespace bin::elf::mod
{
    extern "C" char __section_modules_start[];
    extern "C" char __section_modules_end[];

    namespace
    {
        enum class status { };
        struct entry
        {
            bool internal;

            std::vector<
                std::pair<
                    std::uintptr_t,
                    std::uintptr_t
                >
            > pages;

            ::mod::declare<0> *header;
            std::vector<std::string_view> deps;

            std::function<void ()> init;
            std::function<void ()> fini;

            status status;
        };

        lib::map::flat_hash<std::string_view, entry> modules;
        lib::mutex lock;

        void log_entry(const auto &entry)
        {
            auto ptr = entry.header;
            log::info("elf: internal module: '{}'", ptr->name);
            log::info("elf: - description: '{}'", ptr->description);

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
                log::print("'{}'{}", mod, i == ndeps - 1 ? "" : ", ");
            }
            if (ndeps != 0)
                log::println();
        }

        std::size_t load(bool internal, std::uintptr_t start, std::uintptr_t end, decltype(entry::pages) &&pages, std::function<void ()> &&init, std::function<void ()> &&fini)
        {
            using base_type = ::mod::declare<0>;
            constexpr std::size_t base_size = sizeof(base_type);

            std::size_t nmod = 0;

            start = lib::align_up(start, 8zu);
            end = lib::align_down(end, 8zu);

            auto current = start;
            while (current < end)
            {
                const auto magic = *reinterpret_cast<std::uint64_t *>(current);
                if (magic != base_type::header_magic)
                {
                    current += 8;
                    continue;
                }

                nmod++;

                const auto ptr = reinterpret_cast<base_type *>(current);
                const auto ndeps = ptr->dependencies.ndeps;
                const auto deps = reinterpret_cast<const char *const *>(ptr->dependencies.list);

                auto &entry = modules[ptr->name];
                entry.internal = internal;
                entry.header = ptr;

                entry.pages = std::move(pages);

                entry.init = std::move(init);
                entry.fini = std::move(fini);

                for (std::size_t i = 0; i < ndeps; i++)
                    entry.deps.push_back(deps[i]);

                log_entry(entry);

                current += base_size + ndeps * sizeof(const char *);
            }

            return nmod;
        }

        void load_internal()
        {
            const auto start = reinterpret_cast<std::uintptr_t>(__section_modules_start);
            const auto end = reinterpret_cast<std::uintptr_t>(__section_modules_end);

            auto nmod = load(true, start, end, { }, { }, { });
            log::info("elf: module: found {} internal module{}", nmod, nmod == 1 ? "" : "s");
        }

        bool load(std::shared_ptr<vfs::node> node)
        {
            const std::unique_lock _ { lock };

            static std::uintptr_t base_addr = 0;
            if (base_addr == 0)
                base_addr = vmm::alloc_vpages(vmm::space_type::modules, lib::gib(1));

            decltype(entry::pages) memory;
            std::vector<vmm::flag> flags;

            std::uintptr_t loaded_at = 0;
            auto map = [&loaded_at, &memory, &flags](std::uintptr_t vaddr, Elf64_Xword &size, vmm::flag flag) mutable
            {
                if (loaded_at == 0)
                    loaded_at = base_addr;

                auto &pmap = vmm::kernel_pagemap;

                auto aligned = lib::align_down(vaddr, pmm::page_size);
                if (aligned < vaddr)
                    size += pmm::page_size - (size % pmm::page_size);

                for (std::size_t i = 0; i <= size; i += pmm::page_size)
                {
                    const auto paddr = pmm::alloc<std::uintptr_t>(1, true);
                    if (auto ret = pmap->map(loaded_at + aligned + i, paddr, pmm::page_size, vmm::flag::rw); !ret)
                        lib::panic("could not map memory for a module: {}", magic_enum::enum_name(ret.error()));

                    base_addr += pmm::page_size;
                    memory.emplace_back(loaded_at + aligned + i, paddr);
                    flags.emplace_back(flag);
                }
                return loaded_at + vaddr;
            };

            auto unmap_all = [&memory]() mutable
            {
                auto &pmap = vmm::kernel_pagemap;
                for (auto [vaddr, paddr] : memory)
                {
                    if (auto ret = pmap->unmap(vaddr, pmm::page_size); !ret)
                        lib::panic("could not unmap memory for a module: {}", magic_enum::enum_name(ret.error()));
                    pmm::free(paddr);
                }
                memory.clear();
            };

            log::info("elf: module: loading '{}'", node->name);

            auto &op = node->backing;

            auto ehdr = std::make_unique<Elf64_Ehdr>();
            lib::ensure(op->read(0, std::span {
                reinterpret_cast<std::byte *>(ehdr.get()),
                sizeof(Elf64_Ehdr)
            }) == sizeof(Elf64_Ehdr));

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
            if (ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV)
            {
                log::error("elf: module: invalid os abi");
                return false;
            }
            if (ehdr->e_type != ET_DYN)
            {
                log::error("elf: module: not a shared object");
                return false;
            }

            auto shdrs = std::make_unique<Elf64_Shdr[]>(ehdr->e_shnum);
            lib::ensure(op->read(ehdr->e_shoff, std::span {
                reinterpret_cast<std::byte *>(shdrs.get()),
                ehdr->e_shnum * sizeof(Elf64_Shdr)
            }) == static_cast<std::ssize_t>(ehdr->e_shnum * sizeof(Elf64_Shdr)));

            auto shstrtab = std::make_unique<char[]>(shdrs[ehdr->e_shstrndx].sh_size);
            lib::ensure(op->read(shdrs[ehdr->e_shstrndx].sh_offset, std::span {
                reinterpret_cast<std::byte *>(shstrtab.get()),
                shdrs[ehdr->e_shstrndx].sh_size
            }) == static_cast<std::ssize_t>(shdrs[ehdr->e_shstrndx].sh_size));

            std::uintptr_t modules_start = -1;
            std::size_t modules_size = 0;
            for (std::size_t i = 0; i < ehdr->e_shnum; i++)
            {
                auto &shdr = shdrs[i];
                if (std::string_view { shstrtab.get() + shdr.sh_name } == ".modules")
                {
                    modules_start = shdr.sh_addr;
                    modules_size = shdr.sh_size;
                    break;
                }
            }

            if (modules_start == -1ul)
            {
                log::error("elf: module: no '.modules' section found");
                return false;
            }

            auto phdrs = std::make_unique<Elf64_Phdr[]>(ehdr->e_phnum);
            lib::ensure(op->read(ehdr->e_phoff, std::span {
                reinterpret_cast<std::byte *>(phdrs.get()),
                ehdr->e_phnum * sizeof(Elf64_Phdr)
            }) == ehdr->e_phnum * sizeof(Elf64_Phdr));

            std::uintptr_t dt_strtab = 0;
            std::ptrdiff_t dt_strsz = 0;
            std::uintptr_t dt_symtab = 0;
            std::uintptr_t dt_rela = 0;
            std::ptrdiff_t dt_relasz = 0;
            std::ptrdiff_t dt_relaent = 0;
            std::ptrdiff_t dt_pltrelsz = 0;
            std::uintptr_t dt_jmprel = 0;

            std::uintptr_t dt_init_array = 0;
            std::uintptr_t dt_fini_array = 0;
            std::ptrdiff_t dt_init_arraysz = 0;
            std::ptrdiff_t dt_fini_arraysz = 0;

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

                        phdr.p_vaddr = map(phdr.p_vaddr, phdr.p_memsz, flags);
                        lib::ensure(op->read(phdr.p_offset, std::span {
                            reinterpret_cast<std::byte *>(phdr.p_vaddr),
                            phdr.p_filesz
                        }) == static_cast<std::ssize_t>(phdr.p_filesz));
                        break;
                    }
                    case PT_DYNAMIC:
                    {
                        // should this use p_memsz?
                        auto dyntable = std::make_unique<Elf64_Dyn[]>(phdr.p_filesz / sizeof(Elf64_Dyn));
                        lib::ensure(op->read(phdr.p_offset, std::span {
                            reinterpret_cast<std::byte *>(dyntable.get()),
                            phdr.p_filesz
                        }) == static_cast<std::ssize_t>(phdr.p_filesz));

                        for (std::size_t ii = 0; ii < phdr.p_filesz / sizeof(Elf64_Dyn); ii++)
                        {
                            auto &dyn = dyntable[ii];
                            switch (dyn.d_tag)
                            {
                                case DT_PLTRELSZ: dt_pltrelsz = dyn.d_un.d_val; break;
                                case DT_STRTAB: dt_strtab = dyn.d_un.d_ptr; break;
                                case DT_SYMTAB: dt_symtab = dyn.d_un.d_ptr; break;
                                case DT_RELA: dt_rela = dyn.d_un.d_ptr; break;
                                case DT_RELASZ: dt_relasz = dyn.d_un.d_val; break;
                                case DT_RELAENT: dt_relaent = dyn.d_un.d_val; break;
                                case DT_STRSZ: dt_strsz = dyn.d_un.d_val; break;
                                case DT_JMPREL: dt_jmprel = dyn.d_un.d_ptr; break;
                                case DT_INIT_ARRAY: dt_init_array = dyn.d_un.d_ptr; break;
                                case DT_FINI_ARRAY: dt_fini_array = dyn.d_un.d_ptr; break;
                                case DT_INIT_ARRAYSZ: dt_init_arraysz = dyn.d_un.d_val; break;
                                case DT_FINI_ARRAYSZ: dt_fini_arraysz = dyn.d_un.d_val; break;
                                default: break;
                            }
                        }
                        lib::ensure(
                            dt_strtab != 0 && dt_symtab != 0 && dt_strsz != 0 &&
                            dt_rela != 0 && dt_relasz != 0 && dt_relaent != 0
                        );
                        break;
                    }
                    default:
                        break;
                }
            }

            for (std::size_t i = 0; i < ehdr->e_shnum; i++)
            {
                auto &shdr = shdrs[i];
                shdr.sh_addr += loaded_at;
            }

            auto strtab = std::make_unique<char[]>(dt_strsz);
            lib::ensure(op->read(dt_strtab, std::span {
                reinterpret_cast<std::byte *>(strtab.get()),
                static_cast<std::size_t>(dt_strsz)
            }) == dt_strsz);

            const auto dt_symsz = dt_strtab - dt_symtab;
            auto symtab = std::make_unique<Elf64_Sym[]>(dt_symsz / sizeof(Elf64_Sym));
            lib::ensure(op->read(dt_symtab, std::span {
                reinterpret_cast<std::byte *>(symtab.get()),
                static_cast<std::size_t>(dt_symsz)
            }) == static_cast<std::ssize_t>(dt_symsz));

            auto reloc = [&](Elf64_Rela &rel) -> bool
            {
                const auto &sym = symtab[ELF64_R_SYM(rel.r_info)];
                const char *name = strtab.get() + sym.st_name;

                std::uintptr_t loc = loaded_at + rel.r_offset;
                switch (ELF64_R_TYPE(rel.r_info))
                {
#if defined(__x86_64__)
                    case R_X86_64_64:
                    case R_X86_64_GLOB_DAT:
                    case R_X86_64_JUMP_SLOT:
                    {
                        std::uintptr_t resolved = 0;
                        if (sym.st_shndx == 0)
                        {
                            auto sym = sym::klookup(name);
                            if (sym == sym::empty)
                            {
                                log::error("elf: module: symbol '{}' not found", name);
                                return false;
                            }
                            resolved = sym.address;
                        }
                        else resolved = loaded_at + sym.st_value;

                        *reinterpret_cast<std::uintptr_t *>(loc) = resolved + rel.r_addend;
                        break;
                    }
                    case R_X86_64_RELATIVE:
                        *reinterpret_cast<std::uintptr_t *>(loc) = loaded_at + rel.r_addend;
                        break;
#elif defined(__aarch64__)
#endif
                    default:
                        log::error("elf: module: unknown relocation 0x{:X}", ELF64_R_TYPE(rel.r_info));
                        return false;
                }
                return true;
            };

            auto rela = std::make_unique<Elf64_Rela[]>(dt_relasz / sizeof(Elf64_Rela));
            lib::ensure(op->read(dt_rela, std::span {
                reinterpret_cast<std::byte *>(rela.get()),
                static_cast<std::size_t>(dt_relasz)
            }) == dt_relasz);

            for (std::size_t i = 0; i < dt_relasz / sizeof(Elf64_Rela); i++)
            {
                if (reloc(rela[i]) == false)
                {
                    log::error("elf: module: relocation failed");
                    unmap_all();
                    return false;
                }
            }

            rela = std::make_unique<Elf64_Rela[]>(dt_pltrelsz / sizeof(Elf64_Rela));
            lib::ensure(op->read(dt_jmprel, std::span {
                reinterpret_cast<std::byte *>(rela.get()),
                static_cast<std::size_t>(dt_pltrelsz)
            }) == dt_pltrelsz);

            for (std::size_t i = 0; i < dt_pltrelsz / sizeof(Elf64_Rela); i++)
            {
                if (reloc(rela[i]) == false)
                {
                    log::error("elf: module: relocation failed");
                    unmap_all();
                    return false;
                }
            }

            for (std::size_t i = 0; auto &[vaddr, _] : memory)
            {
                auto &pmap = vmm::kernel_pagemap;
                if (auto ret = pmap->protect(vaddr, pmm::page_size, flags[i]); !ret)
                    lib::panic("could not change mapping flags: {}", magic_enum::enum_name(ret.error()));
                i++;
            }

            // TODO: unmap and return false if no modules?
            auto nmod = load(false,
                loaded_at + modules_start, loaded_at + modules_start + modules_size, std::move(memory),
                [dt_init_array, dt_init_arraysz, loaded_at]
                {
                    auto init_array = reinterpret_cast<void (**)()>(loaded_at + dt_init_array);
                    for (std::size_t i = 0; i < dt_init_arraysz / sizeof(std::uintptr_t); i++)
                    {
                        if (auto func = init_array[i])
                            func();
                    }
                },
                [dt_fini_array, dt_fini_arraysz, loaded_at]
                {
                    auto fini_array = reinterpret_cast<void (**)()>(loaded_at + dt_fini_array);
                    for (std::size_t i = 0; i < dt_fini_arraysz / sizeof(std::uintptr_t); i++)
                    {
                        if (auto func = fini_array[i])
                            func();
                    }
                }
            );
            log::info("elf: module: found {} driver{} in module", nmod, nmod == 1 ? "" : "s");

            return true;
        }

        void load_external()
        {
            auto ret = vfs::resolve(nullptr, "/usr/lib/modules");
            if (!ret || ret->target->backing->stat.type() != stat::type::s_ifdir)
            {
                log::error("elf: module: directory '/usr/lib/modules' not found");
                return;
            }

            auto dir = ret->target;
            dir->fs->populate(dir);

            for (auto &[name, child] : dir->children)
            {
                if (!name.ends_with(".ko") || child->backing->stat.type() != stat::type::s_ifreg)
                    continue;

                load(child);
            }
        }
    } // namespace

    void load()
    {
        load_internal();
        load_external();
    }
} // namespace bin::elf::mod