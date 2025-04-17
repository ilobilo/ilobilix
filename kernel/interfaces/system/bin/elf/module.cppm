// Copyright (C) 2024-2025  ilobilo

export module system.bin.elf:mod;

import lib;
import std;

export namespace bin::elf::mod
{
    bool load(std::uintptr_t addr, std::size_t size);
} // export namespace bin::elf::mod