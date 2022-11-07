// Copyright (C) 2022  ilobilo

#include <drivers/initrd/initrd.hpp>
#include <drivers/pci/pci.hpp>

#include <drivers/fs/devtmpfs.hpp>
#include <drivers/fs/tmpfs.hpp>
#include <drivers/vfs.hpp>

#include <drivers/acpi.hpp>
#include <drivers/term.hpp>
#include <drivers/frm.hpp>
#include <drivers/elf.hpp>
#include <drivers/dtb.hpp>

#include <drivers/proc.hpp>

#include <misc/cxxabi.hpp>
#include <arch/arch.hpp>

#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

proc::process *kernel_proc = nullptr;
void kernel_thread()
{
    tmpfs::init();
    vfs::mount(nullptr, "", "/", "tmpfs");

    devtmpfs::init();
    initrd::init();

    frm::late_init();
    term::late_init();

    elf::module::init();
    elf::module::load(nullptr, "/lib/modules/");
    elf::module::run_all();

    proc::dequeue();
    arch::halt();
}

void main()
{
    pmm::init();
    vmm::init();

    cxxabi::init();
    elf::syms::init();

    #if defined(__aarch64__)
    dtb::init();
    #endif

    frm::init();
    term::init();

    acpi::init();
    arch::init();

    pci::init();
    acpi::enable();

    kernel_proc = new proc::process("Kernel Process");
    kernel_proc->pagemap = vmm::kernel_pagemap;

    proc::enqueue(new proc::thread(kernel_proc, &kernel_thread, 0, false));
    proc::init(true);
}