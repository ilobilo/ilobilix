// Copyright (C) 2022-2024  ilobilo

#include <drivers/initrd/initrd.hpp>
#include <drivers/pci/pci.hpp>

#include <drivers/fs/dev/tty/tty.hpp>
#include <drivers/fs/dev/tty/pty.hpp>
#include <drivers/fs/devtmpfs.hpp>
#include <drivers/fs/tmpfs.hpp>
#include <drivers/vfs.hpp>

#include <drivers/acpi.hpp>
#include <drivers/term.hpp>
#include <drivers/frm.hpp>
#include <drivers/elf.hpp>
#include <drivers/dtb.hpp>
#include <drivers/smp.hpp>

#include <drivers/proc.hpp>

#include <misc/cxxabi.hpp>
#include <arch/arch.hpp>

#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

#include <lib/log.hpp>

proc::process *kernel_proc = nullptr;
void kernel_thread()
{
    pci::init();

    tmpfs::init();
    vfs::mount(nullptr, "", "/", "tmpfs");

    devtmpfs::init();
    initrd::init();

    // Create if it doesn't exist
    vfs::create(nullptr, "/tmp", 01777 | s_ifdir);
    vfs::mount(nullptr, "", "/tmp", "tmpfs", 0, new char[] { "mode=01777" });

    frm::init();
    term::init();
    arch::init();

    tty::init();
    pty::init();

    elf::modules::init();
    elf::modules::load(nullptr, "/usr/lib/modules/");
    elf::modules::run_all();

    log::infoln("Kernel initialisation complete");

    // arch::halt();

    printf("\033[2J\033[H");
    log::to_term = vmm::print_errors = false;

    auto run_prog = [](std::string_view prog, auto &&...args)
    {
        std::string name(prog);
        ([&](std::string_view str)
        {
            name += " ";
            name += str;
        } (args), ...);
        // term::print(name.data());
        // term::printc('\n');

        auto node = std::get<1>(vfs::path2node(nullptr, prog));
        assert(node);

        auto pmap = new vmm::pagemap();
        if (auto ret = elf::exec::load(node->res, pmap, 0); ret.has_value())
        {
            auto proc = new proc::process(name);
            proc->fd_table = std::make_shared<vfs::fd_table>();
            proc->session = std::make_shared<proc::session>();
            proc->parent = kernel_proc;
            proc->pagemap = pmap;

            assert(proc->open(at_fdcwd, "/dev/tty0", o_rdonly, 0, 0) == 0);
            assert(proc->open(at_fdcwd, "/dev/tty0", o_wronly, 0, 1) == 1);
            assert(proc->open(at_fdcwd, "/dev/tty0", o_wronly, 0, 2) == 2);

            std::array argv { prog, std::string_view(args)... };
            std::array envp { "TERM=linux"sv };

            auto [auxv, ld_path] = ret.value();
            if (ld_path.empty() == false)
            {
                auto ld_node = std::get<1>(vfs::path2node(nullptr, ld_path))->reduce(true);
                assert(ld_node);

                if (auto ld_ret = elf::exec::load(ld_node->res, pmap, 0x40000000); ld_ret.has_value())
                {
                    auto [ld_auxv, _] = ld_ret.value();
                    proc::enqueue(new proc::thread(proc, ld_auxv.at_entry, argv, envp, auxv));
                }
                else assert(!"Could not load ld_path");
            }
            else proc::enqueue(new proc::thread(proc, auxv.at_entry, argv, envp, auxv));
        }
        else assert(!"Could not load elf file");
    };

    run_prog("/usr/sbin/init");

    proc::dequeue();
    arch::halt();
}

void kmain()
{
    cxxabi::init();
    elf::syms::init();

    frm::early_init();
    term::early_init();

#if defined(__aarch64__)
    dtb::init();
#endif

    acpi::init();

    time::init();

    arch::early_init();

    kernel_proc = new proc::process("Kernel Process");
    kernel_proc->pagemap = vmm::kernel_pagemap;

    proc::enqueue(new proc::thread(kernel_proc, &kernel_thread, 0, this_cpu()->id));
    proc::init(true);
}
