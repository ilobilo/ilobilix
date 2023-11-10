# Copyright (C) 2022-2023  ilobilo

project('ilobilix', ['c', 'cpp'],
    version : 'v0.1',
    license: 'GPL-3.0',
    default_options : [
        'c_std=gnu17',
        'warning_level=2',
        'werror=true'
    ]
)

c = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')
ld = cxx.get_linker_id()
arch = host_machine.cpu_family()

initrd_dir = meson.source_root() / 'sysroot'
logfile = meson.source_root() / 'log.txt'

# iso_dir = meson.source_root() / 'iso'
iso_dir = '/tmp/ilobilix_tmp_iso_dir'
iso_files = [
    meson.source_root() / 'misc/bg.png',
    meson.source_root() / 'misc/dtb.img',
    meson.source_root() / 'misc/font.bin',
    meson.source_root() / 'misc/limine.cfg'
]

qemu_args = [
    '-cpu', 'max', '-smp', '4', '-m', '512M',
    '-rtc', 'base=localtime', '-serial', 'stdio',
    '-boot', 'order=d,menu=on,splash-time=100',
    '-drive', 'file=' + meson.source_root() / 'misc/nvme.img' + ',format=raw,if=none,id=nvm',
    '-device', 'nvme,serial=nvme,drive=nvm',
    '-nic', 'user,model=rtl8139',
    '-nic', 'user,model=e1000'
]

# { arch : [ ovmf_name, bios, xorriso_args, qemu_args ] }
supported_archs = {
    'x86_64' : [
        'X64', true,
        [
            '-b', 'limine-bios-cd.bin',
            '-no-emul-boot', '-boot-load-size', '4', '-boot-info-table',
            '--efi-boot', 'limine-uefi-cd.bin',
            '-efi-boot-part', '--efi-boot-image', '--protective-msdos-label'
        ],
        [
            '-M', 'q35', '-audiodev', 'id=audio,driver=alsa',
            '-machine', 'pcspk-audiodev=audio',
        ]
    ],
    'aarch64' : [
        'AA64', false,
        [
            '--efi-boot', 'limine-uefi-cd.bin',
            '-efi-boot-part', '--efi-boot-image', '--protective-msdos-label'
        ],
        ['-M', 'virt', '-device', 'ramfb']
    ]
}

if not supported_archs.has_key(arch)
    error('Unknown architecture ' + arch)
endif

ovmf_id = supported_archs[arch][0]
supports_bios = supported_archs[arch][1]
xorriso_args = supported_archs[arch][2]
qemu_args += supported_archs[arch][3]
qemu = 'qemu-system-' + arch

if get_option('vnc')
    qemu_args += ['-vnc', '127.0.0.1:1']
endif

gdb = get_option('gdb')
if get_option('qemu_debug') or gdb
    qemu_args += [
        '-no-reboot', '-no-shutdown',
        '-d', 'int', '-D', logfile,
        '-monitor', 'telnet:127.0.0.1:12345,server,nowait'
    ]
    if arch == 'x86_64'
        qemu_args += ['-M', 'smm=off']
    endif
    if gdb == true
        qemu_args += ['-s', '-S']
    endif
elif not get_option('noaccel')
    qemu_args += ['-M', 'accel=kvm:hvf:whpx:haxm:tcg']
endif

incs = [
    include_directories('include'),
    include_directories('include/std'),
    include_directories('include/std/stubs'),
    include_directories('include/libc'),
    include_directories('include/kernel'),
    # include_directories('include/kernel/arch' / arch),
    include_directories('include/modules')
]

macros = [
    '-DLVL5_PAGING=' + (get_option('5lvl_paging') ? '1' : '0'),
    '-DSYSCALL_DEBUG=' + (get_option('syscall_debug') ? '1' : '0')
]

warnigns = [
    '-Wno-error=#warnings',
    '-Wno-builtin-macro-redefined',
    '-Wno-macro-redefined'
]

add_global_arguments([
    warnigns, macros,
    '-ffreestanding',
    '-fno-stack-protector',
    '-fno-omit-frame-pointer',
    '-fno-pic',
    '-fno-pie',
    '-Ofast'
], language : ['c', 'cpp'])

add_global_arguments([
    '-std=gnu++23',
    '-fno-rtti',
    '-fno-exceptions',
    '-fsized-deallocation',
    '-Wno-unused-parameter',
    '-Wno-non-virtual-dtor',
    '-Wno-frame-address',
    '-DMAGIC_ENUM_NO_STREAMS=1',
    '-D__cpp_concepts=202207L'
], language : 'cpp')

cmake = import('cmake')

phmap_opts = cmake.subproject_options()
phmap_opts.add_cmake_defines({
    'PHMAP_BUILD_EXAMPLES' : false,
    'PHMAP_BUILD_TESTS' : false,
    'PHMAP_INSTALL' : false
})

limine = subproject('limine', default_options : 'arch=' + arch)
deps = [
    cmake.subproject('parallel_hashmap', options : phmap_opts).dependency('phmap'),
    cmake.subproject('frozen').dependency('frozen'),

    dependency('compiler_rt_builtins', default_options : 'custom_includes=' + meson.source_root() / 'include/libc'),
    dependency('demangler', default_options : 'only_itanium=true'),
    dependency('frigg', default_options : 'frigg_no_install=true'),
    dependency('magic_enum', default_options : 'test=false'),

    dependency('libstdcxx_headers'),
    dependency('limine_terminal'),
    dependency('smart_ptr'),
    dependency('string'),
    dependency('printf'),
    dependency('limine'),
    dependency('cwalk'),
    dependency('veque'),
    dependency('lai'),
    dependency('fmt')
]

inc_deps = [ ]
foreach dep : deps
    inc_deps += dep.partial_dependency(includes : true)
endforeach

ext_mods = [ ]
mod_deps = [ ]
subdir('modules')

kernel_elf = [ ]
subdir('kernel')

fs = import('fs')
if not fs.is_dir(initrd_dir)
    error('Directory \'' + fs.name(initrd_dir) + '\' not found!')
endif

no_ext_mods = ext_mods.length() == 0
initrd = custom_target('initrd',
    input : ext_mods,
    output : 'initrd.img.gz',
    command : [
        'rm', '-rf', initrd_dir / 'usr/lib/modules', '&&',
        'mkdir', '-p', initrd_dir / 'usr/lib/modules', '&&',
        no_ext_mods ? [ ] : ['cp', '@INPUT@', initrd_dir / 'usr/lib/modules/', '&&'],
        'tar', '--format', 'posix', '-czf', '@OUTPUT@', '-C', initrd_dir, './'
    ],
    build_by_default : true,
    console : true,
    install : false,
    depends : ext_mods
)

iso_files += limine.get_variable('limine_binaries')
iso = custom_target('iso',
    input : [kernel_elf, initrd],
    output : 'image.iso',
    command : [
        'rm', '-rf', iso_dir, '&&',
        'mkdir', '-p', iso_dir, '&&',
        'cp', '@INPUT0@', iso_dir / 'kernel.elf', '&&',
        'cp', '@INPUT1@', iso_dir / 'initrd.img.gz', '&&',
        'cp', iso_files, iso_dir, '&&',
        'xorriso', '-as', 'mkisofs', xorriso_args, iso_dir, '-o', '@OUTPUT@', '&&',
        supports_bios ? [ limine.get_variable('limine_exec'), 'bios-install', '@OUTPUT@', '&&' ] : [],
        'rm', '-rf', iso_dir
    ],
    build_by_default : true,
    console : true,
    install : false,
    depends : [kernel_elf, initrd]
)

if supports_bios
    run_bios = custom_target('run_bios',
        input : iso,
        output : 'run_bios',
        command : [qemu, qemu_args, '-cdrom', '@INPUT@'],
        build_by_default : true,
        console : true,
        install : false,
        depends : iso
    )
endif

ovmf_binaries = subproject('ovmf_binaries')
ovmf = ovmf_binaries.get_variable(ovmf_id)

run_uefi = custom_target('run_uefi',
    input : iso,
    output : 'run_uefi',
    command : [qemu, qemu_args, '-cdrom', '@INPUT@', '-bios', ovmf],
    build_by_default : not supports_bios,
    console : true,
    install : false,
    depends : iso
)

norun = custom_target('norun',
    output : 'norun',
    command : 'true',
    install : false,
    depends : iso
)