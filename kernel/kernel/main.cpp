// Copyright (C) 2022  ilobilo

#include <drivers/initrd/initrd.hpp>
#include <drivers/pci/pci.hpp>

#include <drivers/fs/devtmpfs.hpp>
#include <drivers/fs/tmpfs.hpp>
#include <drivers/vfs.hpp>

#include <drivers/serial.hpp>
#include <drivers/acpi.hpp>
#include <drivers/term.hpp>
#include <drivers/frm.hpp>
#include <drivers/elf.hpp>

#include <misc/cxxabi.hpp>
#include <arch/arch.hpp>

#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

#include <lib/log.hpp>

void main()
{
    serial::early_init();

    mm::pmm::init();
    mm::vmm::init();

    frm::init();
    term::init();

    cxxabi::init();
    elf::syms::init();

    acpi::init();
    arch::init();

    pci::init();
    acpi::enable();

    tmpfs::init();
    vfs::mount(nullptr, "", "/", "tmpfs");

    devtmpfs::init();
    initrd::init();

    elf::module::init();
    elf::module::load(nullptr, "/lib/modules/");
    elf::module::run_all();
}