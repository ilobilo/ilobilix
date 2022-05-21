// Copyright (C) 2022  ilobilo

#include <drivers/syms/syms.hpp>
#include <mm/pmm/pmm.hpp>
#include <mm/vmm/vmm.hpp>
#include <lib/string.hpp>
#include <lib/vector.hpp>
#include <lib/errno.hpp>
#include <lib/misc.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <lib/elf.h>
#include <main.hpp>
#include <cstddef>
#include <cdi.h>

namespace elf
{
    namespace module
    {
        vector<cdi_driver*> drivers;
        static uint64_t base_addr = 0;
        static lock_t lock;

        uint64_t map(uint64_t size)
        {
            if (base_addr == 0) base_addr = align_up(mm::pmm::mem_top + hhdm_offset, mm::pmm::block_size);
            size = align_up(size, mm::pmm::block_size);

            uint64_t loadaddr = base_addr;
            base_addr += size;

            uint64_t paddr = mm::pmm::alloc<uint64_t>(size / mm::pmm::block_size);
            mm::vmm::kernel_pagemap->mapMemRange(loadaddr, paddr, size);

            return loadaddr;
        }

        void unmap(uint64_t loadaddr, uint64_t size)
        {
            size = align_up(size, mm::pmm::block_size);
            if (loadaddr == base_addr - size) base_addr = loadaddr;

            void *ptr = reinterpret_cast<void*>(mm::vmm::kernel_pagemap->virt2phys(loadaddr));
            mm::pmm::free(ptr, size / mm::pmm::block_size);
            mm::vmm::kernel_pagemap->unmapMemRange(loadaddr, size);
        }

        extern "C" void print(const char *str)
        {
            log::println("%s", str);
        }

        [[clang::no_sanitize("alignment")]] bool load(uint64_t address, uint64_t size)
        {
            auto header = reinterpret_cast<Elf64_Ehdr*>(address);
            if (memcmp(header->e_ident, ELFMAG, 4))
            {
                log::error("ELF: Invalid magic!");
                return false;
            }
            if (header->e_ident[EI_CLASS] != ELFCLASS64)
            {
                log::error("ELF: Invalid class!");
                return false;
            }
            if (header->e_type != ET_REL)
            {
                log::error("ELF: Not a relocatable object!");
                return false;
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
                    if (section->sh_size == 0) continue;
                    section->sh_addr = map(section->sh_size);
                    section->sh_offset = section->sh_addr - loadaddr;
                    memset(reinterpret_cast<void*>(section->sh_addr), 0, section->sh_size);
                }
                else
                {
                    section->sh_addr = loadaddr + section->sh_offset;
                    if (section->sh_addralign && (section->sh_addr & (section->sh_addralign - 1)))
                    {
                        log::warn("ELF: Module not aligned correctly! 0x%lX %ld", section->sh_addr, section->sh_addralign);
                    }
                }
            }

            #if defined(__aarch64__)
            auto aarch64_imm_adr = [](uint32_t val) -> uint32_t
            {
                uint32_t low  = (val & 0x03) << 29;
                uint32_t high = ((val >> 2) & 0x07FFFF) << 5;
                return low | high;
            };

            auto aarch64_imm_12 = [](uint32_t val) -> uint32_t
            {
                return (val & 0x0FFF) << 10;
            };
            #endif

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
                        uintptr_t val = 0;

                        if (sym->st_shndx == SHN_UNDEF)
                        {
                            auto addr = syms::lookup(name);
                            if (addr == 0)
                            {
                                log::error("ELF: Could not find kernel symbol \"%s\"", name);
                                return false;
                            }
                            val = addr;
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
                                *static_cast<uint64_t*>(loc) = val - reinterpret_cast<uintptr_t>(loc);
                                break;
                            case R_X86_64_32:
                                *static_cast<uint32_t*>(loc) = val;
                                break;
                            case R_X86_64_PC32:
                            case R_X86_64_PLT32:
                                *static_cast<uint32_t*>(loc) = val - reinterpret_cast<uintptr_t>(loc);
                                break;
                            #elif defined(__aarch64__)
                            case 275:
                                *static_cast<uint32_t*>(loc) = *static_cast<uint32_t*>(loc) | aarch64_imm_adr(((val) >> 12) - (reinterpret_cast<uintptr_t>(loc) >> 12));
                                break;
                            case 286:
                                *static_cast<uint32_t*>(loc) = *static_cast<uint32_t*>(loc) | aarch64_imm_12(((val) >> 3) & 0x01FF);
                                break;
                            case 282:
                            case 283:
                                *static_cast<uint32_t*>(loc) = *static_cast<uint32_t*>(loc) | (((val - reinterpret_cast<uintptr_t>(loc)) >> 2) & 0x03FFFFFF);
                                break;
                            case 257:
                                *static_cast<uint64_t*>(loc) = val;
                                break;
                            case 258:
                                *static_cast<uint32_t*>(loc) = val;
                                break;
                            #endif
                            default:
                                log::error("ELF: Unsupported relocation %ld found!", ELF64_R_TYPE(entry->r_info));
                                unmap(loadaddr, size);
                                return false;
                        }
                    }
                }
            }

            auto strtable = reinterpret_cast<char*>(loadaddr + sections[header->e_shstrndx].sh_offset);
            bool found = false;

            for (size_t i = 0; i < header->e_shnum; i++)
            {
                auto section = &sections[i];
                if (section->sh_size != 0 && (section->sh_size % sizeof(void*)) == 0 && !strcmp(DRIVER_SECTION, strtable + section->sh_name))
                {
                    found = true;
                    uint64_t offset = section->sh_addr;
                    for (size_t d = 0; d < (section->sh_size / sizeof(void*)); d++)
                    {
                        auto driver = reinterpret_cast<cdi_driver*>(*reinterpret_cast<uintptr_t*>(offset));
                        log::info("Found driver: \"%s\", Type: %d", driver->name, driver->type);
                        drivers.push_back(driver);
                        offset += sizeof(void*);
                    }
                    break;
                }
            }

            if (found == false)
            {
                log::error("ELF: Could not find any drivers in module!");
                unmap(loadaddr, size);
                return false;
            }

            return true;
        }
    } // namespace module
    namespace exec
    {
    } // namespace exec
} // namespace elf