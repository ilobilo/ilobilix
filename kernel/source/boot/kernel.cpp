// Copyright (C) 2024  ilobilo

import ilobilix;
import std;

extern "C" std::uint8_t kernel_stack[boot::kernel_stack_size] { };
extern "C" auto kernel_stack_top = kernel_stack + boot::kernel_stack_size;

extern "C" void kmain()
{
    serial::early_init();
    boot::check_requests();

    memory::init();
    serial::init();
    cxxabi::construct();

    acpi::early();
    arch::init();
    pmm::reclaim();
    acpi::init();

    // lib::panic("get stick bugged lol");

    arch::halt();
}