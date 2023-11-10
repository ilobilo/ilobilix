# Copyright (C) 2022-2023  ilobilo

src = files(
    'drivers/fs/dev/tty/tty.cpp',
    'drivers/fs/dev/tty/pty.cpp',

    'drivers/fs/dev/streams.cpp',

    'drivers/fs/devtmpfs.cpp',
    'drivers/fs/tmpfs.cpp',

    'drivers/initrd/ilar.cpp',
    'drivers/initrd/initrd.cpp',
    'drivers/initrd/ustar.cpp',

    'drivers/pci/pci_acpi.cpp',
    'drivers/pci/pci_ecam.cpp',
    'drivers/pci/pci.cpp',

    'drivers/acpi.cpp',
    'drivers/dtb.cpp',
    'drivers/elf.cpp',
    'drivers/fd.cpp',
    'drivers/frm.cpp',
    'drivers/proc.cpp',
    'drivers/smp.cpp',
    'drivers/term.cpp',
    'drivers/vfs.cpp',

    'init/kernel.cpp',
    'init/main.cpp',

    'lib/net/ipv4/icmp.cpp',
    'lib/net/arp.cpp',
    'lib/net/net.cpp',

    'lib/alloc.cpp',
    'lib/errno.cpp',
    'lib/event.cpp',
    'lib/log.cpp',
    'lib/panic.cpp',
    'lib/time.cpp',
    'lib/trace.cpp',

    'libc/ctype.cpp',
    'libc/stdio.cpp',
    'libc/stdlib.cpp',
    'libc/string.cpp',
    'libc/wchar.cpp',

    'misc/cxxabi.cpp',
    'misc/ubsan.cpp',

    'mm/mmap.cpp',
    'mm/pmm.cpp',
    'mm/vmm.cpp',

    'syscall/mm.cpp',
    'syscall/proc.cpp',
    'syscall/vfs.cpp'
)

c_args = get_option('kernel_cflags').split()
cxx_args = get_option('kernel_cxxflags').split()

ld_args = [
    '-Wl,-T,' + meson.current_source_dir() / 'linker-' + arch + '.ld',
    '-Wl,-z,max-page-size=0x1000',
    '-nostdlib',
    '-static'
]

args = [ ]
if get_option('kernel_ubsan')
    args += '-fsanitize=undefined'
endif

subdir('arch' / arch)

kernel_elf = executable('ilobilix.elf', src,
    dependencies : [deps, mod_deps],
    include_directories : incs,
    c_args : [c_args, args],
    cpp_args : [cxx_args, args],
    link_args : ld_args,
    install : false
)