<!-- # Kernel project v2 -->
<!-- # MINUX Is Not Unix -->
<!-- # MOSINUX OS Is Not Unix -->
# Ilobilix
Second attempt at making an OS

## [LICENSE](LICENSE)

## Building And Running

Make sure you have following programs installed:
* meson
* ninja
* clang/clang++
* lld
* llvm
* xorriso
* tar
* xbstrap
* qemu-system-x86_64
* qemu-system-aarch64

Note: currently only llvm, clang and lld are supported\
Note: you may need more tools to build the sysroot, such as flex, bison, automake, autoconf, etc

If are on Debian based system (Ubuntu, linux mint, Pop_os! etc) you can install most of them with this command:\
``sudo apt install clang lld llvm xorriso tar qemu-system-x86 qemu-system-arm``\
For meson and ninja, first make sure you have python installed and then run:\
``sudo python -m pip install meson ninja xbstrap``

Follow these steps to build and run the os:
1. Clone this repo with:\
``git clone --single-branch --branch=master --depth=1 https://github.com/ilobilo/ilobilix``

2. Currently you have to manually build the sysroot:
* Set the architecture in boostrap.yml
* ``mkdir build-sysroot``
* ``pushd build-sysroot``
* ``xbstrap init ..``
* ``xbstrap install base``
* ``rsync -rl system-root ../initrd``
* ``popd``

3. Go to the root directory of the cloned repo and run:\
``meson setup builddir --cross-file cross-files/meson-clang-(x86_64/aarch64).cross-file -Doptions=values``

4. Build and run the kernel:\
``ninja -C builddir``

Note: optionally you can add run_bios or run_efi to ninja arguments\
Note: on aarch64, only run_efi is supported\
Note: if firmware type is not specified and architecture supports bios mode, run_bios will be used, if it doesn't, then run_efi

### Options
|  Project options  | Default Value |               Description                |
| ----------------- | ------------- | ---------------------------------------- |
| kernel_cflags     |               | Extra c compiler arguments for kernel    |
| kernel_cxxflags   |               | Extra cpp compiler arguments for kernel  |
| modules_cflags    |               | Extra c compiler arguments for modules   |
| modules_cxxflags  |               | Extra cpp compiler arguments for modules |
| kernel_ubsan      | true          | Enable ubsanitizer in kernel             |
| modules_ubsan     | false         | Enable ubsanitizer in modules            |
| 5lvl_paging       | false         | Enable 5 level paging in kernel          |
| qemu_debug        | false         | Enable interrupt logging in qemu and starts monitor on telnet:127.0.0.1:12345. Enables 'noaccel' |
| gdb               | false         | Add -s -S to qemu. Enables 'qemu_debug'  |
| noaccel           | false         | Disable qemu accelerators                |
| vnc               | false         | Start qemu VNC server on 127.0.0.1:5901  |
<!-- ```
Project options    Default Value                 Description
-----------------  -------------  ----------------------------------------
kernel_cflags                     Extra c compiler arguments for kernel
kernel_cxxflags                   Extra cpp compiler arguments for kernel
modules_cflags                    Extra c compiler arguments for modules
modules_cxxflags                  Extra cpp compiler arguments for modules
kernel_ubsan       true           Enable ubsanitizer in kernel
modules_ubsan      false          Enable ubsanitizer in modules
5lvl_paging        false          Enable 5 level paging in kernel
qemu_debug         false          Enable interrupt logging in qemu and starts monitor on telnet:127.0.0.1:12345. Enables 'noaccel'
gdb                false          Add -s -S to qemu. Enables 'qemu_debug'
noaccel            false          Disable qemu accelerators
vnc                false          Start VNC server on 127.0.0.1:5901
``` -->

## Discord server
https://discord.gg/fM5GK3RpS7

## Resources/projects:
* meson: https://mesonbuild.com/
* osdev wiki: https://wiki.osdev.org
* osdev discord server: https://discord.gg/RnCtsqD
* managarm: https://github.com/managarm/managarm
* toaruOS: https://github.com/klange/toaruos
* LemonOS: https://github.com/LemonOSProject/LemonOS
* Sigma: https://github.com/sigma-os/Sigma
* Luna: https://github.com/thomtl/Luna
* vinix: https://github.com/vlang/vinix
* lyre: https://github.com/lyre-os/lyre
* limine: https://github.com/limine-bootloader/limine
* lai: https://github.com/managarm/lai
* cxxshim: https://github.com/managarm/cxxshim
* frigg: https://github.com/managarm/frigg
* mlibc: https://github.com/managarm/mlibc
* magic_enum: https://github.com/Neargye/magic_enum
* fmt: https://github.com/fmtlib/fmt
* printf: https://github.com/eyalroz/printf
* smart_ptr: https://github.com/X-czh/smart_ptr
* veque: https://github.com/Shmoopty/veque
* cwalk: https://github.com/likle/cwalk
* unifont: https://ftp.gnu.org/gnu/unifont/unifont-14.0.02
* terminal: https://github.com/V01D-NULL/limine-terminal-port
* ilar: https://github.com/ilobilo/ilar
* ovmf: https://github.com/ilobilo/ovmf-binaries

## TODO

- [x] Serial
- [x] GDT
- [x] IDT
- [x] TSS
- [x] PCI
- [x] PCIe
- [x] MSI
- [ ] MSI-X
- [x] Modules x86_64
- [ ] Modules aarch64
- [ ] DTB
- [ ] PS/2
- [x] PMM
- [x] VMM (5 and 4 level)
- [x] Heap
- [x] ACPI
- [x] LAPIC
- [x] IOAPIC
- [x] LAI
- [x] VFS
- [x] TMPFS
- [x] DEVTMPFS
- [ ] PROCFS
- [ ] SYSFS
- [x] USTAR
- [x] ILAR
- [ ] EchFS
- [ ] EXT2
- [ ] FAT32
- [x] HPET
- [x] PIT
- [x] RTC
- [x] LAPIC Timer
- [ ] Generic Timer
- [x] SMP
- [x] Scheduler x86_64
- [ ] Scheduler aarch64
- [x] System calls x86_64
- [ ] System calls aarch64
- [ ] Permissions
- [x] MMAP
- [x] FDs
- [ ] CDEV
- [ ] FBDEV
- [ ] TTY/PTY
- [x] ELF
- [x] Userspace
- [ ] Signals
- [x] mlibc
- [x] bash
- [x] coreutils
- [ ] readline
- [ ] ncurses
- [ ] DOOM
- [ ] gcc
- [ ] tcc