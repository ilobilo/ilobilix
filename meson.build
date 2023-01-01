project('ilobilix', ['c', 'cpp'],
    version : 'v0.1',
    license: 'GPL-3.0',
    default_options : [
        # 'cpp_std=gnu++20',
        'c_std=gnu17',
        'warning_level=2',
        'werror=true'
    ]
)

c = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')
ld = cxx.get_linker_id()
arch = host_machine.cpu_family()

initrd_dir = meson.source_root() / 'initrd'
logfile = meson.source_root() / 'log.txt'

# iso_dir = meson.source_root() / 'iso'
iso_dir = '/tmp/ilobilix_tmp_iso_dir'
iso_files = [
    meson.source_root() / 'misc/bg.bmp',
    meson.source_root() / 'misc/dtb.img',
    meson.source_root() / 'misc/font.bin',
    meson.source_root() / 'limine.cfg'
]

qemu_args = [
    '-cpu', 'max', '-smp', '4', '-m', '512M',
    '-rtc', 'base=localtime', '-serial', 'stdio',
    '-boot', 'order=d,menu=on,splash-time=100'
]

# { arch : [ libgcc, ovmf, bios, qemu_args ] }
supported_archs = {
    'x86_64' : [
        'x86_64-no-red-zone',
        'X64', true,
        [
            '-M', 'q35', '-audiodev', 'id=audio,driver=alsa',
            '-machine', 'pcspk-audiodev=audio',
            # '-device', 'piix3-ide,id=ide',
            # '-drive', 'id=disk,file=$(DISK1),format=raw,if=none',
            # '-device', 'ide-hd,drive=disk,bus=ide.0',
            # '-drive', 'format=raw,file=$(DISK0)',
            '-net', 'nic,model=rtl8139',
            '-net', 'user,hostfwd=tcp::4321-:4321'
        ]
    ],
    'aarch64' : [
        'aarch64', 'AA64', false,
        ['-cpu', 'cortex-a72', '-M', 'virt', '-device', 'ramfb']
        # ['-M', 'virt', '-device', 'ramfb']
    ]
}

if not supported_archs.has_key(arch)
    error('Unknown architecture ' + arch)
endif

libgcc = supported_archs[arch][0]
ovmf_id = supported_archs[arch][1]
supports_bios = supported_archs[arch][2]
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
    include_directories('include/kernel/arch' / arch),
    include_directories('include/modules')
]

macros = [ ]
if get_option('5lvl_paging')
    macros += '-DLVL5_PAGING=1'
else
    macros += '-DLVL5_PAGING=0'
endif

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
    '-std=gnu++2b',
    '-fno-rtti',
    '-fno-exceptions',
    '-fsized-deallocation',
    '-Wno-unused-parameter',
    '-Wno-non-virtual-dtor',
    '-DMAGIC_ENUM_NO_STREAMS=1',
    '-D__cplusplus=202101L',
    '-D__cpp_concepts=202002L',
    '-D__cpp_lib_format=202207L'
], language : 'cpp')

limine = subproject('limine')
deps = [
    import('cmake').subproject('frozen').dependency('frozen'),
    dependency('libgcc_binaries', default_options : 'lib=' + libgcc),
    dependency('frigg', default_options : 'frigg_no_install=true'),
    dependency('demangler', default_options : 'only_itanium=true'),
    dependency('libstdcxx_headers'),
    dependency('limine_terminal'),
    dependency('limine'),
    dependency('lai'),
    dependency('fmt'),
    dependency('printf'),
    dependency('magic_enum'),
    dependency('smart_ptr'),
    dependency('veque'),
    dependency('cwalk')
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

if not import('fs').is_dir(initrd_dir)
    error('Directory \'initrd\' not found!')
endif

no_ext_mods = ext_mods.length() == 0
initrd = custom_target('initrd',
    input : ext_mods,
    output : 'initrd.img.gz',
    command : [
        'rm', '-rf', initrd_dir / 'usr/lib/modules', '&&',
        'mkdir', initrd_dir / 'usr/lib/modules', '&&',
        no_ext_mods ? [ ] : ['cp', '@INPUT@', initrd_dir / 'usr/lib/modules/', '&&'],
        'tar', '--format', 'posix', '-czf', '@OUTPUT@', '-C', initrd_dir, './'
    ],
    build_by_default : true,
    console : true,
    install : false,
    depends : ext_mods
)

iso_files += limine.get_variable('binaries')
iso = custom_target('iso',
    input : [kernel_elf, initrd],
    output : 'image.iso',
    command : [
        'mkdir', '-p', iso_dir, '&&',
        'cp', '@INPUT0@', iso_dir / 'kernel.elf', '&&',
        'cp', '@INPUT1@', iso_dir / 'initrd.img.gz', '&&',
        'cp', iso_files, iso_dir, '&&',
        'xorriso', '-as', 'mkisofs', '-b', 'limine-cd.bin',
        '-no-emul-boot', '-boot-load-size', '4', '-boot-info-table',
        '--efi-boot', 'limine-cd-efi.bin', '-efi-boot-part',
        '--efi-boot-image', '--protective-msdos-label',
        iso_dir, '-o', '@OUTPUT@', '&&',
        limine.get_variable('deploy'), '@OUTPUT@', '&&',
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

run_efi = custom_target('run_efi',
    input : iso,
    output : 'run_efi',
    command : [qemu, qemu_args, '-cdrom', '@INPUT@', '-bios', ovmf],
    build_by_default : not supports_bios,
    console : true,
    install : false,
    depends : iso
)