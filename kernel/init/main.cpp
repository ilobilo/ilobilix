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

#include <lib/log.hpp>

proc::process *kernel_proc = nullptr;
void kernel_thread()
{
    tmpfs::init();
    vfs::mount(nullptr, "", "/", "tmpfs");

    // Create if it doesn't exist
    vfs::create(nullptr, "/tmp", 01777 | s_ifdir);
    vfs::mount(nullptr, "", "/tmp", "tmpfs");

    devtmpfs::init();
    initrd::init();

    frm::late_init();
    term::late_init();

    elf::module::init();
    elf::module::load(nullptr, "/usr/lib/modules/");
    elf::module::run_all();

    printf("\033[2J\033[H");
    log::to_term = false;

    // TMP start

    auto run_prog = [](std::string_view prog, auto &&...args)
    {
        auto node = std::get<1>(vfs::path2node(nullptr, prog));
        assert(node);

        auto pmap = new vmm::pagemap();
        if (auto ret = elf::exec::load(node->res, pmap, 0); ret.has_value())
        {
            auto proc = new proc::process("Process");
            proc->pagemap = pmap;

            auto cons_node = std::get<1>(vfs::path2node(nullptr, "/dev/console"));
            assert(cons_node);

            proc->res2num(cons_node->res, 0, 0, true);
            proc->res2num(cons_node->res, 0, 1, true);
            proc->res2num(cons_node->res, 0, 2, true);

            std::array argv { prog, std::string_view(args)... };
            std::array envp { "TERM=vt100"sv };

            auto [auxv, ld_path] = ret.value();
            if (ld_path.empty() == false)
            {
                auto ld_node = std::get<1>(vfs::path2node(nullptr, ld_path))->reduce(true);
                assert(ld_node);

                if (auto ld_ret = elf::exec::load(ld_node->res, pmap, 0x40000000); ld_ret.has_value())
                {
                    auto [ld_auxv, _] = ld_ret.value();
                    proc::enqueue(new proc::thread(proc, ld_auxv.at_entry, 0, argv, envp, auxv));
                }
                else assert(!"Could not load ld_path");
            }
            else proc::enqueue(new proc::thread(proc, auxv.at_entry, 0, argv, envp, auxv));
        }
        else assert(!"Could not load elf file");
    };

    // run_prog("/bin/bash", "/bin/bashbug");
    // run_prog("/bin/bash", "/test.sh");
    // run_prog("/usr/bin/cat", "/etc/passwd", "/etc/group");
    run_prog("/bin/bash");
    // TMP end

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

    proc::enqueue(new proc::thread(kernel_proc, &kernel_thread, 0));
    proc::init(true);
}