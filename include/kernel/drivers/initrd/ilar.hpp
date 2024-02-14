// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <cstdint>

namespace ilar
{
    inline constexpr uint64_t PATH_LENGTH = 128;
    inline constexpr char ILAR_SIGNATURE[] = "ILAR";

    enum filetypes
    {
        ILAR_REGULAR = 0,
        ILAR_DIRECTORY = 1,
        ILAR_SYMLINK = 2
    };

    struct [[gnu::packed]] header
    {
        char signature[5];
        char name[PATH_LENGTH];
        char link[PATH_LENGTH];
        uint64_t size;
        uint8_t type;
        uint32_t mode;
    };

    bool validate(uintptr_t address);
    void init(uintptr_t address);
} // namespace ilar