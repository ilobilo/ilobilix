// Copyright (C) 2022-2023  ilobilo

#include <lib/log.hpp>
#include <module.hpp>

// Always put DRIVER() before init and fini functions
// Adding __init__ and __fini__ is necessary for DRIVER() to function
// test1 depends on nvme. test2 depends on test1 and rtl8139
DRIVER(test1, init, fini, "nvme")

__init__ bool init()
{
    log::infoln("Hello from first noarch test driver!");
    return true;
}

__fini__ bool fini()
{
    log::infoln("{}", fmt::format("Goodbye from first {} test driver!", "noarch"));
    return true;
}

DRIVER(test2, init4, fini4, "test1", "rtl8139")

__init__ bool init4()
{
    log::infoln("Hello from second noarch test driver!");
    return true;
}

__fini__ bool fini4()
{
    log::infoln("Goodbye from second {} test driver!", "noarch");
    return true;
}