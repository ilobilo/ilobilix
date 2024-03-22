// Copyright (C) 2022-2024  ilobilo

#include <lib/panic.hpp>
#include <lib/log.hpp>
#include <cctype>

#include <frg/macros.hpp>

extern "C"
{
    void FRG_INTF(log)(const char *msg)
    {
        log::infoln("FRG: {}{}", char(std::toupper(*msg)), msg + 1);
    }

    void FRG_INTF(panic)(const char *cstring)
    {
        panic(cstring);
    }
} // extern "C"