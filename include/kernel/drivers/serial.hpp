// Copyright (C) 2022-2024  ilobilo

#pragma once

namespace serial
{
    void printc(char c);
    char readc();

    void early_init();
    void second_early_init();

    void init();
} // namespace serial