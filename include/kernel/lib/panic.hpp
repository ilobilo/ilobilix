// Copyright (C) 2022  ilobilo

#pragma once

[[noreturn]] void panic(const char *file, int line, const char *func, const char *message);
[[noreturn]] void panic(const char *message);

[[noreturn]] extern "C" void abort();

#define PANIC(msg) panic(__FILE__, __LINE__, __PRETTY_FUNCTION__, msg)