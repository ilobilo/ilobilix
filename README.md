# Kernel project v2
Second attempt at making an OS

## [LICENSE](LICENSE)

## TODO

### System
- [x] GDT
- [x] IDT
- [x] TSS
- [x] PCI
- [x] PCIe
- [x] MSI
- [ ] MSI-X
- [x] Modules

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
- [ ] Scheduler
- [ ] IPC
- [ ] SHM
- [ ] Signals

<!-- ### Partition tables
- [ ] MBR
- [ ] GPT -->

### Filesystems
- [ ] VFS
- [ ] TMPFS
- [ ] DEVTMPFS
- [ ] PROCFS
- [ ] SYSFS
- [ ] USTAR
<!-- - [ ] Ext2
- [ ] Fat32
- [ ] ISO9660
- [ ] NTFS -->

### Userspace
- [ ] System calls
- [ ] ELF
- [ ] Userspace
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

## Building And Running

Make sure you have following programs installed:
* Clang
* lld
* llvm
* Make
* Xorriso
* Wget
* Tar
* Qemu x86-64

If are on Debian based system (Ubuntu, linux mint, Pop_os! etc) you can install them with this command:\
```sudo apt install clang lld llvm xorriso wget tar qemu-system-x86``

Follow these steps to build and run the os:
1. Clone this repo with:\
``git clone --single-branch --branch=master --depth=1 https://github.com/ilobilo/kernelv2``

2. Go to the root directory of the cloned repo and run:\
``make uefi -j$(nproc --all)`` To run in UEFI mode\
``make bios -j$(nproc --all)`` To run in BIOS mode

Note: If you are on Termux, add ``VNC=1`` to arguments and connect to ``127.0.0.1:5901`` with VNC viewer:\

### Options
* ``MODUBSAN=1``: Enable UBSAN in modules
* ``NOUBSAN=1``: Disable UBSAN
* ``NOCLEAN=1``: Don't clean the source after compiling
* ``NORUN=1``: Don't run the kernel, just compile
* ``NOACCEL=1``: Disable accelerators
* ``DEBUG=1``: Disable accelerators and enable QEMU logging
* ``GDB=1``: If DEBUG is on, enable QEMU GDB
* ``VNC=1``: Disable QEMU GUI window and run VNC on port 5901
* ``CFLAGS``, ``CXXFLAGS``, ``LDFLAGS``, ``ASFLAGS``: Arguments for CC, CXX, LD and AS respectively (applies for both kernel and modules)
* ``KERNEL_CFLAGS``, ``KERNEL_CXXFLAGS``, ``KERNEL_LDFLAGS``, ``KERNEL_ASFLAGS``: Same as previous, but for kernel
* ``MODULE_CFLAGS``, ``MODULE_CXXFLAGS``, ``MODULE_LDFLAGS``, ``MODULE_ASFLAGS``: Same as previous, but for modules

## Discord server
https://discord.gg/fM5GK3RpS7

## Resources used:
* Osdev wiki: https://wiki.osdev.org
* Osdev discord server: https://discord.gg/RnCtsqD
* Limine: https://github.com/limine-bootloader/limine
* Lai: https://github.com/managarm/lai
* Frigg: https://github.com/managarm/frigg
* MLibc: https://github.com/managarm/mlibc
* Libsmarter: https://github.com/managarm/libsmarter
* Managarm: https://github.com/managarm/managarm
* ToaruOS: https://github.com/klange/toaruos
* LemonOS: https://github.com/LemonOSProject/LemonOS
* Sigma: https://github.com/sigma-os/Sigma
* Luna: https://github.com/thomtl/Luna
* Vinix: https://github.com/vlang/vinix
* Lyre: https://github.com/lyre-os/lyre
* Printf: https://github.com/eyalroz/printf
* CWalk: https://github.com/likle/cwalk
* Unifont: https://ftp.gnu.org/gnu/unifont/unifont-14.0.02
* Terminal: https://github.com/V01D-NULL/limine-terminal-port
* OVMF: https://efi.akeo.ie/