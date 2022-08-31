override SUPARCHS := x86_64
ARCH ?= x86_64

ifeq (,$(findstring $(ARCH),$(SUPARCHS)))
    $(error Architecture $(ARCH) not supported, please use one of the following: $(SUPARCHS))
endif

ifeq ($(ARCH),x86_64)
    override EFI_ARCH := X64
    override LIBGCC := libgcc-x86_64-no-red-zone.a
    override TARGET := x86_64-pc-none-elf
else
endif

ROOTDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
KERNELDIR := $(ROOTDIR)/kernel
MODULEDIR := $(ROOTDIR)/modules
INITRDDIR := $(ROOTDIR)/initrd
EXTDEPDIR := $(ROOTDIR)/extdeps
ISO_ROOT := $(ROOTDIR)/iso_root

MODSRCDIR := $(MODULEDIR)/source/
MODDESTDIR := $(INITRDDIR)/lib/modules/
MODSUBDIRS := $(realpath $(wildcard $(MODSRCDIR)/$(ARCH)/*/)) $(realpath $(wildcard $(MODSRCDIR)/noarch/*/))

MODULES := $(foreach i,$(MODSUBDIRS),$(i)/$(notdir $(i)).ko)
KERNEL_MODULES := $(foreach i,$(MODSUBDIRS),$(if $(realpath $(i)/EXTERNAL),,$(i)/$(notdir $(i)).ko))
EXTERN_MODULES := $(foreach i,$(MODSUBDIRS),$(if $(realpath $(i)/EXTERNAL),$(i)/$(notdir $(i)).ko))

LIMINEDIR ?= $(EXTDEPDIR)/limine
OVMF ?= $(EXTDEPDIR)/ovmf/OVMF.fd

BACKGROUND ?= $(ROOTDIR)/misc/bg.bmp
TERMFONT ?= $(ROOTDIR)/misc/font.bin
SFNFONT ?= $(ROOTDIR)/misc/font.sfn

KERNEL ?= $(KERNELDIR)/kernel.elf

ISO ?= $(ROOTDIR)/image.iso
DISK0 ?= $(ROOTDIR)/disk0.img
DISK1 ?= $(ROOTDIR)/disk1.img

LOGFILE ?= $(ROOTDIR)/log.txt

VERSION ?= 0
NET_DEBUG ?= 1
LVL5_PAGING ?= 0

CC = clang
CXX = clang++
LD = ld.lld

CFLAGS ?= 
CXXFLAGS ?= 
ASFLAGS ?= 
LDFLAGS ?= 

CVERSION = gnu1x
CXXVERSION = gnu++2b

LIMINE ?= $(LIMINEDIR)/limine-deploy
XORRISO ?= xorriso

INITRD ?= $(ROOTDIR)/initrd.img.gz
TAR ?= tar

CXXFILT ?= llvm-cxxfilt
QEMU = qemu-system-$(ARCH)

override QEMUFLAGS := -cpu max -smp 4 -m 512M \
	-rtc base=localtime -serial stdio         \
	-boot d -cdrom $(ISO)

ifeq ($(ARCH),x86_64)
    override QEMUFLAGS += -M q35                        \
        -audiodev id=audio,driver=alsa                  \
        -machine pcspk-audiodev=audio                   \
        -device piix3-ide,id=ide                        \
        -drive id=disk,file=$(DISK1),format=raw,if=none \
        -device ide-hd,drive=disk,bus=ide.0             \
        -drive format=raw,file=$(DISK0)                 \
        -net nic,model=rtl8139                          \
        -net user,hostfwd=tcp::4321-:4321
else
endif

ifdef VNC
    override QEMUFLAGS += -vnc 127.0.0.1:1
endif

ifdef DEBUG
    override QEMUFLAGS += -no-reboot -no-shutdown     \
        -d int -D $(LOGFILE)                          \
        -monitor telnet:127.0.0.1:12345,server,nowait

    ifeq ($(ARCH),x86_64)
        override QEMUFLAGS += -M smm=off
    endif

    ifdef GDB
        override QEMUFLAGS += -s -S
    endif
else
    ifndef NOACCEL
        override QEMUFLAGS += -M accel=kvm:hvf:whpx:haxm:tcg
    endif
endif

XORRISOFLAGS = -as mkisofs -b limine-cd.bin          \
	-no-emul-boot -boot-load-size 4 -boot-info-table \
	--efi-boot limine-cd-efi.bin -efi-boot-part      \
	--efi-boot-image --protective-msdos-label

override INCLUDES :=                            \
	-I$(ROOTDIR)/include/                       \
	-I$(ROOTDIR)/include/std/                   \
	-I$(ROOTDIR)/include/libc/                  \
	-I$(ROOTDIR)/include/kernel/                \
	-I$(ROOTDIR)/include/kernel/arch/$(ARCH)/   \
	-I$(EXTDEPDIR)/tlsf/                        \
	-I$(EXTDEPDIR)/limine/                      \
	-I$(EXTDEPDIR)/printf/src/                  \
	-I$(EXTDEPDIR)/lai/include/                 \
	-I$(EXTDEPDIR)/cwalk/include/               \
	-I$(EXTDEPDIR)/frigg/include/               \
	-I$(EXTDEPDIR)/veque/include/               \
	-I$(EXTDEPDIR)/smart_ptr/include/           \
	-I$(EXTDEPDIR)/cxxshim/stage2/include/      \
	-I$(EXTDEPDIR)/limine-terminal-port/source/

override GLOBALLIBS :=                           \
	-L$(EXTDEPDIR)/libgcc-binaries/ -l:$(LIBGCC)

export

.PHONY: all
all: bios

.PHONY: uefi
uefi: extdeps iso-clean modules-clean
	@$(MAKE) -s modules
	@$(MAKE) -s $(KERNEL)
	@$(MAKE) -s $(INITRD)
	@$(MAKE) -s $(ISO)
	@$(MAKE) -s clean
	@$(MAKE) -s run-uefi

.PHONY: bios
bios: extdeps iso-clean modules-clean
	@$(MAKE) -s modules
	@$(MAKE) -s $(KERNEL)
	@$(MAKE) -s $(INITRD)
	@$(MAKE) -s $(ISO)
	@$(MAKE) -s clean
	@$(MAKE) -s run-bios

.PHONY: extdeps
extdeps:
	@$(MAKE) -sC $(ROOTDIR)/extdeps

.PHONY: extdeps-clean
extdeps-clean:
	@$(MAKE) -sC $(ROOTDIR)/extdeps clean

.PHONY: modules
modules:
	@$(MAKE) -sC $(ROOTDIR)/modules
	@mkdir -p $(MODDESTDIR)
	@$(if $(strip $(EXTERN_MODULES)),cp $(EXTERN_MODULES) $(MODDESTDIR))

.PHONY: modules-clean
modules-clean: initrd-clean
	@$(MAKE) -sC $(ROOTDIR)/modules clean
	@rm -f $(MODDESTDIR)/*.ko

$(KERNEL):
	@$(MAKE) -sC $(KERNELDIR)

.PHONY: kernel-clean
kernel-clean:
	@$(MAKE) -sC $(KERNELDIR) clean

$(INITRD):
	@printf "TAR\t%s\n" $(INITRD:$(ROOTDIR)/%=%)
	$(TAR) --format posix -czf $(INITRD) -C $(ROOTDIR)/initrd/ ./

.PHONY: initrd-clean
initrd-clean:
	@rm -f $(INITRD)

$(ISO):
	@mkdir -p $(ISO_ROOT)
	@cp $(KERNEL) $(INITRD) $(BACKGROUND) $(TERMFONT) $(SFNFONT) $(ROOTDIR)/limine.cfg $(LIMINEDIR)/limine.sys \
		$(LIMINEDIR)/limine-cd.bin $(LIMINEDIR)/limine-cd-efi.bin $(ISO_ROOT)

	@printf "XORRISO\t%s\n" $(ISO:$(ROOTDIR)/%=%)
	@$(XORRISO) $(XORRISOFLAGS) $(ISO_ROOT) -o $(ISO) 2> /dev/null || echo "\e[31mFailed to build iso!\e[0m"

	@printf "LIMINE\t%s\n" $(ISO:$(ROOTDIR)/%=%)
	@$(LIMINE) $(ISO) 2> /dev/null || echo "\e[32mFailed to install Limine!\e[0m"

.PHONY: iso-clean
iso-clean:
	@rm -f $(ISO)

.PHONY: run
run: run-bios

.PHONY: run-uefi
run-uefi:
ifndef NORUN
	@echo "\nBooting in uefi mode...\n"
	@$(QEMU) $(QEMUFLAGS) -drive if=pflash,format=raw,unit=0,file=$(OVMF) | $(CXXFILT)
endif

run-bios:
ifndef NORUN
	@echo "\nBooting in bios mode...\n"
	@$(QEMU) $(QEMUFLAGS) | $(CXXFILT)
endif

clean:
ifndef NOCLEAN
	@$(MAKE) -s kernel-clean
	@rm -rf $(LOGFILE) $(ISO_ROOT)
endif

distclean:
ifndef NOCLEAN
	@$(MAKE) -s clean modules-clean initrd-clean extdeps-clean
endif