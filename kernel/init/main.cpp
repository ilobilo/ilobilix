// Copyright (C) 2022-2023  ilobilo

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

    devtmpfs::init();
    initrd::init();

    // Create if it doesn't exist
    vfs::create(nullptr, "/tmp", 01777 | s_ifdir);
    vfs::mount(nullptr, "", "/tmp", "tmpfs", 0, new char[] { "mode=01777" });

    frm::late_init();
    term::late_init();
    arch::late_init();

    tty::init();
    pty::init();

    elf::modules::init();
    elf::modules::load(nullptr, "/usr/lib/modules/");
    elf::modules::run_all();

    printf("\033[2J\033[H");
    log::to_term = vmm::print_errors = false;

    // TMP start

    auto run_prog = [](std::string_view prog, auto &&...args)
    {
        std::string name(prog);
        ([&](std::string_view str)
        {
            name += " ";
            name += str;
        } (args), ...);
        term::print(name.data());
        term::printc('\n');

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

        while (proc::processes.size() != 1)
            proc::yield();
    };

    // run_prog("/bin/bash", "/bin/bashbug");
    // run_prog("/bin/bash", "/test.sh");
    // run_prog("/usr/bin/cat", "/etc/group");

    // run_prog("/usr/bin/stat", "/passwd");
    // run_prog("/usr/bin/stat", "-L", "/passwd");
    // run_prog("/usr/bin/stat", "/text");
    // run_prog("/usr/bin/chmod", "+x", "/text");
    // run_prog("/usr/bin/stat", "/text");

    // run_prog("/usr/bin/stat", "/text");
    // run_prog("/usr/bin/ls", "--color=auto", "/");
    // run_prog("/usr/bin/chmod", "+x", "/text");
    // run_prog("/usr/bin/stat", "/text");
    // run_prog("/usr/bin/ls", "--color=auto", "/");

    // run_prog("/usr/bin/cat", "/dev/random");

    // run_prog("/getline");

    // run_prog("/usr/bin/ls", "--color=auto", "/");
    // run_prog("/usr/bin/ls", "--color=auto", "/dev");
    // run_prog("/usr/bin/ls", "--color=auto", "/dev/pts");

    // run_prog("/usr/bin/stat", "/dev/console");
    // run_prog("/usr/bin/stat", "/dev/tty0");
    // run_prog("/usr/bin/stat", "/dev/tty");

    // devtmpfs::add_dev("testnull", makedev(1, 3), 0666 | s_ifchr);
    // run_prog("/usr/bin/ls", "--color=auto", "/dev");
    // run_prog("/usr/bin/stat", "/dev/null");
    // run_prog("/usr/bin/stat", "/dev/testnull");
    // run_prog("/usr/bin/stat", "/null");

    // run_prog("/usr/bin/ln", "/etc/passwd", "/passwd");
    // run_prog("/usr/bin/readlink", "/passwd");
    // run_prog("/usr/bin/cat", "/passwd");
    // run_prog("/usr/bin/rm", "/passwd");
    // run_prog("/usr/bin/cat", "/passwd");

    // run_prog("/usr/bin/cat", "/usr/share/doc/bash/README");
    // run_prog("/usr/bin/ls", "-a", "--color=auto", "/usr/include/");
    // run_prog("/bin/bash");
    // run_prog("/fork");
    run_prog("/init");
    // TMP end

    proc::dequeue();
    arch::halt();
}

void kmain()
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