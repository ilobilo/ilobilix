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
* qemu-system-x86_64
* qemu-system-aarch64

Note: currently only llvm, clang and lld are supported

If are on Debian based system (Ubuntu, linux mint, Pop_os! etc) you can install most of them with this command:\
``sudo apt install clang lld llvm xorriso tar qemu-system-x86 qemu-system-arm``\
For meson and ninja, first make sure you have python installed and then run:\
``sudo python -m pip install meson ninja``

Follow these steps to build and run the os:
1. Clone this repo with:\
``git clone --single-branch --branch=master --depth=1 https://github.com/ilobilo/ilobilix``

2. Go to the root directory of the cloned repo and run:\
``meson setup builddir --cross-file cross-files/meson-clang-(x86_64/aarch64).cross-file -Doptions=values``\
3. Build and run the kernel:\
``ninja -C builddir``

Note: optionally you can add run_bios or run_efi to ninja arguments\
Note: on aarch64, only run_efi is supported\
Note: without specifying firmware type, if architecture supports bios mode, it will default to run_bios, else run_efi

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

### Misc
- [x] GDT
- [x] IDT
- [x] TSS
- [x] PCI
- [x] PCIe
- [x] MSI
- [ ] MSI-X
- [x] Modules
- [ ] DTB

### Memory
- [x] PMM
- [x] VMM (5 and 4 level)
- [x] Heap

### ACPI
- [x] ACPI
- [x] LAPIC
- [x] IOAPIC
- [x] LAI

<!-- ### Device drivers
#### Audio
- [ ] PC speaker
- [ ] AC97
- [ ] SB16

#### I/O
- [ ] PS/2 Keyboard
- [ ] PS/2 Mouse
- [x] COM

#### VMs
- [ ] VMWare Tools
- [ ] VBox Guest Additions
- [ ] Virtio

#### Storage
- [ ] FDC
- [ ] IDE
- [ ] SATA
- [ ] NVMe
- [ ] Virtio block

#### Network
- [ ] RTL8139
- [ ] RTL8169
- [ ] E1000
- [ ] Virtio network

#### USB
- [ ] UHCI
- [ ] OHCI
- [ ] EHCI
- [ ] XHCI -->

### Timers
- [x] HPET
- [x] PIT
- [x] RTC
- [x] LAPIC Timer

### Tasking
- [x] SMP
- [x] Scheduler
- [ ] Signals

<!-- ### Partition tables
- [ ] MBR
- [ ] GPT -->

### Filesystems
- [x] VFS
- [x] TMPFS
- [x] DEVTMPFS
- [ ] PROCFS
- [ ] SYSFS
- [x] USTAR
- [x] ILAR
<!-- - [ ] Ext2
- [ ] Fat32
- [ ] ISO9660
- [ ] NTFS -->

### Userspace
- [x] System calls
- [ ] MMAP
- [ ] FDs
- [x] ELF
- [x] Userspace
- [ ] Libc
- [ ] Bash
- [ ] DOOM

### Network stack
- [ ] Ethernet
- [ ] ARP
- [ ] IPv4
- [ ] ICMPv4
- [ ] TCP
- [ ] UDP
- [ ] DHCP
- [ ] HTTP
- [ ] Telnet
- [ ] SSL
- [ ] Or just LWIP