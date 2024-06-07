# Copyright (C) 2022-2024  ilobilo

src += files(
    'cpu/cpu.cpp',
    'cpu/gdt.cpp',
    'cpu/idt.cpp',
    'cpu/idt.S',
    'cpu/ioapic.cpp',
    'cpu/lapic.cpp',
    'cpu/pic.cpp',

    'drivers/pci/pci_legacy.cpp',
    'drivers/pci/pci.cpp',

    'drivers/timers/hpet.cpp',
    'drivers/timers/pit.cpp',
    'drivers/timers/rtc.cpp',
    'drivers/timers/tsc.cpp',

    'drivers/proc.cpp',
    'drivers/serial.cpp',
    'drivers/smp.cpp',

    'lib/memory.S',

    'mm/vmm.cpp',

    'syscall/syscall.cpp',
    'syscall/syscall.S',
    'syscall/arch.cpp',

    'arch.cpp'
)

args += [
    '-march=x86-64',
    '-msoft-float',
    '-mno-mmx',
    '-mno-sse',
    '-mno-sse2',
    '-mno-red-zone',
    '-mcmodel=kernel'
]

ld_args += ['-Wl,-z,noexecstack']