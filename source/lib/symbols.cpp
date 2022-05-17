// Copyright (C) 2022  ilobilo

#include <lib/string.hpp>
#include <lib/log.hpp>
#include <lib/elf.h>
#include <main.hpp>

namespace symbols
{
    struct symtable_t
    {
        std::string_view name;
        uint64_t addr;
    };

    symtable_t *symbol_table = nullptr;
    size_t entries = 0;

    uint64_t lookup(const char *name)
    {
        auto name_view = std::string_view(name);
        for (size_t i = 0; i < entries; i++)
        {
            symtable_t entry = symbol_table[i];
            if (entry.name == name_view) return entry.addr;
        }
        return 0;
    }

    void init()
    {
        log::info("Initialising kernel symbol table...");

        uint64_t kfile = reinterpret_cast<uint64_t>(kernel_file_request.response->kernel_file->address);
        auto header = reinterpret_cast<Elf64_Ehdr*>(kfile);
        auto sections = reinterpret_cast<Elf64_Shdr*>(kfile + header->e_shoff);
        Elf64_Sym *symtab = nullptr;
        char *strtab = nullptr;

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

        size_t j, min_idx;
        for (size_t i = 0; i < entries - 1; i++)
        {
            min_idx = i;
            for (j = i + 1; j < entries; j++) if (symtab[j].st_value < symtab[min_idx].st_value) min_idx = j;

            Elf64_Sym temp = symtab[min_idx];
            symtab[min_idx] = symtab[i];
            symtab[i] = temp;
        }

        while (symtab[0].st_value == 0)
        {
            symtab++;
            entries--;
        }

        symbol_table = new symtable_t[entries];

        for (size_t i = 0, entriesbck = entries; i < entriesbck; i++)
        {
            symtable_t sym
            {
                std::string_view(&strtab[symtab[i].st_name]),
                symtab[i].st_value
            };
            symbol_table[i] = sym;
        }
    }
} // namespace symbols