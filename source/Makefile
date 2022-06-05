SOURCEDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
ROOTDIR := $(shell dirname $(SOURCEDIR))
MODULEDIR := $(realpath $(ROOTDIR)/modules)

EXTLIBDIR := $(ROOTDIR)/extlibs
LIMINEDIR ?= $(EXTLIBDIR)/limine

BACKGROUND ?= $(ROOTDIR)/misc/bg.bmp
TERMFONT ?= $(ROOTDIR)/misc/font.bin
SFNFONT ?= $(ROOTDIR)/misc/font.sfn

KERNEL ?= $(SOURCEDIR)/kernel.elf
ISO_ROOT ?= $(ROOTDIR)/iso_root

ISO ?= $(ROOTDIR)/image.iso
DISK0 ?= $(ROOTDIR)/disk0.img
DISK1 ?= $(ROOTDIR)/disk1.img

LOGFILE ?= $(ROOTDIR)/log.txt

VERSION = 0
NET_DEBUG = 1
LVL5_PAGING = 0

CC = clang
CPP = clang++
LD = ld.lld

CXXFILT ?= llvm-cxxfilt

LIMINE ?= $(LIMINEDIR)/limine-deploy
XORRISO ?= xorriso

LDSCRIPT = $(SOURCEDIR)/linker.ld
QEMU ?= qemu-system-x86_64

QEMUFLAGS = -smp 4 -m 512M -serial stdio -boot d -cdrom $(ISO)

QEMUFLAGS += -cpu max -M q35 -rtc base=localtime    \
	-audiodev id=audio,driver=alsa                  \
	-machine pcspk-audiodev=audio                   \
	-device piix3-ide,id=ide                        \
	-drive id=disk,file=$(DISK1),format=raw,if=none \
	-device ide-hd,drive=disk,bus=ide.0             \
	-drive format=raw,file=$(DISK0)                 \
	-net nic,model=rtl8139                          \
	-net user,hostfwd=tcp::1234-:1234
#	-net nic -net tap,ifname=tap0,script=no

ifdef VNC
QEMUFLAGS += -vnc 127.0.0.1:1
endif

ifdef DEBUG
QEMUFLAGS += -no-reboot -no-shutdown              \
	-d int -D $(LOGFILE) -M smm=off               \
	-monitor telnet:127.0.0.1:12345,server,nowait
else
ifndef NOKVM
QEMUFLAGS += -enable-kvm
endif
endif

XORRISOFLAGS = -as mkisofs -b limine-cd.bin          \
	-no-emul-boot -boot-load-size 4 -boot-info-table \
	--efi-boot limine-cd-efi.bin -efi-boot-part      \
	--efi-boot-image --protective-msdos-label

CFLAGS = -Ofast -pipe -Werror -Wall -Wextra -F dwarf -g  \
	-DNET_DEBUG=$(NET_DEBUG) -DLVL5_PAGING=$(LVL5_PAGING)

CPPFLAGS = $(CFLAGS) -Wno-c99-designator -Wno-register \
	-Wno-unused-parameter -Wno-deprecated-volatile

LDFLAGS = 
ASFLAGS = 

INCLUDES :=                                       \
	-I$(SOURCEDIR)/                               \
	-I$(MODULEDIR)/cdi/                           \
	-I$(MODULEDIR)/libc/                          \
	-I$(SOURCEDIR)/libc/                          \
	-I$(EXTLIBDIR)/limine/                        \
	-I$(EXTLIBDIR)/functional/                    \
	-I$(EXTLIBDIR)/printf/src/                    \
	-I$(EXTLIBDIR)/lai/include/                   \
	-I$(EXTLIBDIR)/cwalk/include/                 \
	-I$(EXTLIBDIR)/scalable-font2/                \
	-I$(EXTLIBDIR)/cxxshim/stage2/include/        \
	-I$(EXTLIBDIR)/limine-terminal-port/source/   \
	-I$(EXTLIBDIR)/cxxshim/std-coroutine/include/

override INTERNALLDFLAGS =   \
	-T$(LDSCRIPT)            \
	-nostdlib                \
	-zmax-page-size=0x1000   \
	-static

override INTERNALFLAGS :=      \
	-target x86_64-pc-none-elf \
	-fno-pic                   \
	-mabi=sysv                 \
	-mno-80387                 \
	-mno-mmx                   \
	-mno-3dnow                 \
	-mno-sse                   \
	-mno-sse2                  \
	-mcmodel=kernel	           \
	$(INCLUDES)

override INTERNALCFLAGS :=  \
	$(INTERNALFLAGS)        \
	-std=gnu17              \
	-ffreestanding          \
	-fno-stack-protector    \
	-fno-omit-frame-pointer \
	-mno-red-zone           \
	$(INCLUDES)

override INTERNALCPPFLAGS := \
	$(INTERNALFLAGS)         \
	-std=gnu++20             \
	-ffreestanding           \
	-fno-exceptions          \
	-fno-stack-protector     \
	-fno-omit-frame-pointer  \
	-fno-rtti                \
	-mno-red-zone            \
	$(INCLUDES)

override INTERNALASFLAGS := \
	$(INTERNALFLAGS)        \
	-mgeneral-regs-only     \
	-masm=intel

override CFILES := $(shell find $(SOURCEDIR)/ -type f -name '*.c') \
	$(shell find $(EXTLIBDIR)/limine-terminal-port/source/ -type f -name '*.c' 2> /dev/null) \
	$(shell find $(EXTLIBDIR)/lai/ -type f -name '*.c' 2> /dev/null)
#	$(shell find $(EXTLIBDIR)/cwalk/src/ -type f -name '*.c' 2> /dev/null)

override CPPFILES := $(shell find $(SOURCEDIR)/ -type f -name '*.cpp')
override ASFILES := $(shell find $(SOURCEDIR)/ -type f -name '*.S')

override OBJ = $(CFILES:.c=.o)
override OBJ += $(CPPFILES:.cpp=.o)
override OBJ += $(ASFILES:.S=_S.o)

.PHONY: all
all: $(KERNEL)
	$(MAKE) iso
	$(MAKE) clean run

.PHONY: bios
bios: $(KERNEL)
	$(MAKE) iso
	$(MAKE) clean run-bios

$(KERNEL):
	$(MAKE) $(OBJ)
	@printf "LD\t%s\n" $(KERNEL:$(ROOTDIR)/%=%)
	$(LD) $(LDFLAGS) $(INTERNALLDFLAGS) $(OBJ) -o $@

%.o: %.c
	@printf "CC\t%s\n" $(<:$(ROOTDIR)/%=%)
ifndef NOUBSAN
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -fsanitize=undefined -c $< -o $@
else
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@
endif

%.o: %.cpp
	@printf "CPP\t%s\n" $(<:$(ROOTDIR)/%=%)
ifndef NOUBSAN
	$(CPP) $(CPPFLAGS) $(INTERNALCPPFLAGS) -fsanitize=undefined -c $< -o $@
else
	$(CPP) $(CPPFLAGS) $(INTERNALCPPFLAGS) -c $< -o $@
endif

%_S.o: %.S
	@printf "AS\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CC) $(ASFLAGS) $(INTERNALASFLAGS) -c $< -o $@

iso:
	cp $(KERNEL) $(BACKGROUND) $(TERMFONT) $(SFNFONT) $(ROOTDIR)/limine.cfg $(LIMINEDIR)/limine.sys \
		$(LIMINEDIR)/limine-cd.bin $(LIMINEDIR)/limine-cd-efi.bin $(ISO_ROOT)

	@printf "XORRISO\t%s\n" $(ISO:$(ROOTDIR)/%=%)
	$(XORRISO) $(XORRISOFLAGS) $(ISO_ROOT) -o $(ISO) 2> /dev/null || echo "\e[31mFailed to build iso!\e[0m"

	@printf "LIMINE\t%s\n" $(ISO:$(ROOTDIR)/%=%)
	$(LIMINE) $(ISO) 2> /dev/null || echo "\e[31mFailed to install Limine!\e[0m"

clean:
ifndef NOCLEAN
	rm -rf $(KERNEL) $(OBJ) $(ISO_ROOT)
endif

run:
ifndef NORUN
	@echo "\nBooting iso in uefi mode...\n"
	$(QEMU) $(QEMUFLAGS) -drive if=pflash,format=raw,unit=0,file=$(ROOTDIR)/ovmf/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(ROOTDIR)/ovmf/OVMF_VARS-pure-efi.fd | $(CXXFILT)
endif

run-bios:
ifndef NORUN
	@echo "\nBooting iso in bios mode...\n"
	$(QEMU) $(QEMUFLAGS) | $(CXXFILT)
endif