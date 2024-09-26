# Ilobilix
Second attempt at making an OS

## [LICENSE](LICENSE)

## Building and Running

Make sure you have following programs installed:
* xmake
* clang/clang++ (tested versions >= 19)
* clang-tools (for clang-scan-deps)
* lld
* llvm
* xorriso
* tar
* xbstrap
* qemu-system-x86_64
* qemu-system-aarch64

Note: you may need more packages to build the sysroot, such as ``flex bison automake autoconf autopoint gperf help2man texinfo libgmp-dev libmpc-dev libmpfr-dev`` etc

* If you are on a Debian based system you can install some of the required packages with this command:\
``sudo apt install xorriso tar qemu-system-x86 qemu-system-arm``
* Install llvm, lld, clang and clang-tools from: https://apt.llvm.org
* To install ``xmake``, simply follow instructions on this link: https://xmake.io/#/getting_started?id=installation
* For ``xbstrap``, make sure you have python pip installed, then run: ``python -m pip install xbstrap``

Follow these steps to build and run the os:
1. Clone this repository with: ``git clone --depth=1 https://github.com/ilobilo/ilobilix``
2. Manually build the sysroot:
   * Set the architecture in ``userspace/boostrap.yml``
   * ``mkdir build-sysroot``
   * ``pushd build-sysroot``
   * ``xbstrap init ../userspace``
   * ``xbstrap install base``
   * If symlink named ``sysroot`` does not exist in ``$PROJECT_ROOT/userspace`` that links to ``$BUILD_SYSROOT_DIR/system-root``, create it with:\
   ``ln -s $BUILD_SYSROOT_DIR/system-root $PROJECT_ROOT/userspace/sysroot``\
   For example, run this command from project root: \
   ``ln -s ../build-sysroot/system-root userspace/sysroot``
   * ``popd``
3. Optionally configure the kernel with: ``xmake f --menu -y``
4. To quickly switch between architectures use: ``xmake f --arch=<arch> -y``
5. Build and run the kernel: ``xmake build -j$(nproc) && xmake run`` or just ``xmake run -j$(nproc)``
6. Default run target is ``uefi``. Possible values are: ``bios`` ``bios-debug`` ``uefi`` and ``uefi-debug``. For example: ``xmake run uefi-debug``
7. Rebuild the kernel if configuration is changed: ``xmake build -r --all -j$(nproc)``

## Discord Server
https://discord.gg/fM5GK3RpS7

## Resources and Projects:
* xmake: https://xmake.io
* osdev wiki: https://osdev.wiki
* osdev discord server: https://discord.gg/RnCtsqD
* managarm: https://github.com/managarm/managarm
* limine: https://github.com/limine-bootloader/limine
* uACPI: https://github.com/UltraOS/uACPI
* mlibc: https://github.com/managarm/mlibc
* tart: https://github.com/qookei/tart
* toaruOS: https://github.com/klange/toaruos
* LemonOS: https://github.com/LemonOSProject/LemonOS
* Sigma: https://github.com/sigma-os/Sigma
* Luna: https://github.com/thomtl/Luna
* vinix: https://github.com/vlang/vinix
* lyre: https://github.com/lyre-os/lyre
* unifont: https://ftp.gnu.org/gnu/unifont/unifont-14.0.02
* frigg: https://github.com/managarm/frigg
* magic_enum: https://github.com/Neargye/magic_enum
* frozen: https://github.com/serge-sans-paille/frozen
* fmt: https://github.com/fmtlib/fmt
* printf: https://github.com/eyalroz/printf
* cwalk: https://github.com/likle/cwalk
* cxx headers: https://github.com/osdev0/freestnd-cxx-hdrs
* c headers: https://github.com/osdev0/freestnd-c-hdrs
* hashmap: https://github.com/ilobilo/parallel-hashmap
* smart_ptr: https://github.com/ilobilo/smart_ptr
* veque: https://github.com/ilobilo/veque
* terminal: https://github.com/ilobilo/limine-terminal
* demangler: https://github.com/ilobilo/demangler
* ilar: https://github.com/ilobilo/ilar
* ovmf: https://github.com/ilobilo/ovmf-binaries
* compiler-rt: https://github.com/ilobilo/compiler-rt-builtins
* and more...

## TODO

- [x] Serial
- [x] GDT
- [x] IDT
- [x] TSS
- [x] PCI
- [x] PCIe
- [x] MSI
- [x] MSI-X
- [x] Modules x86_64
- [ ] Modules aarch64
- [x] DTB
- [x] PS/2
- [x] PMM
- [x] VMM (5 and 4 level)
- [x] Heap
- [x] ACPI
- [x] LAPIC
- [x] IOAPIC
- [x] uACPI
- [x] VFS
- [x] TMPFS
- [x] DEVTMPFS
- [ ] PROCFS
- [ ] SYSFS
- [x] USTAR
- [x] ILAR
- [ ] AHCI
- [x] NVME
- [ ] Block Device Interface
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
- [x] CDEV
- [x] FBDEV
- [x] TTY
- [ ] PTY
- [x] ELF
- [x] Userspace
- [ ] Signals
- [x] mlibc
- [x] RTL8139
- [ ] RTL8169
- [x] E1000 (100E 153A and 10EA)
- [x] Ethernet
- [x] ARP
- [x] IPv4
- [x] ICMPv4
- [ ] TCP
- [ ] UDP

## Packages

* libiconv
* libintl
* zlib
* file
* ncurses
* readline
* bash
* coreutils
* nano