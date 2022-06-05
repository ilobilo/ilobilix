THISDIR := $(realpath $(shell pwd))
ROOTDIR := $(realpath $(THISDIR)/../../..)
MODULEDIR := $(realpath $(ROOTDIR)/modules)
SOURCEIR := $(realpath $(ROOTDIR)/source)
EXTLIBDIR := $(realpath $(ROOTDIR)/extlibs)
MODDESTDIR := $(realpath $(ROOTDIR)/initrd/lib/modules)

MODULE := $(MODDESTDIR)/$(notdir $(THISDIR)).ko

CC = clang
CPP = clang++
LD = ld.lld

CFLAGS = -O2 -pipe -Werror -Wall -Wextra -Wno-unused-parameter
CPPFLAGS = $(CFLAGS)
LDFLAGS = 
ASFLAGS = 

-include Makefile.mk

INCLUDES := -I$(MODULEDIR)/cdi -I$(MODULEDIR)/libc

override INTERNALLDFLAGS = -nostdlib -static -relocatable

override INTERNALFLAGS :=      \
	-target x86_64-pc-none-elf \
	-fno-pic                   \
	-mabi=sysv                 \
	-mno-80387                 \
	-mno-mmx                   \
	-mno-3dnow                 \
	-mno-sse                   \
	-mno-sse2                  \
	-mcmodel=large	           \
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


override CFILES := $(shell find $(THISDIR)/ -type f -name '*.c')
override CPPFILES := $(shell find $(THISDIR)/ -type f -name '*.cpp')
override ASFILES := $(shell find $(THISDIR)/ -type f -name '*.S')

override OBJ = $(CFILES:.c=.o)
override OBJ += $(CPPFILES:.cpp=.o)
override OBJ += $(ASFILES:.S=_S.o)

ifeq ($(MODULE),$(wildcard $(MODULE)))
all:
else
all: $(MODULE)
endif
ifndef NOCLEAN
	rm -f $(OBJ)
endif

$(MODULE): $(OBJ)
	@printf "LD\t%s\n" $(MODULE:$(ROOTDIR)/%=%)
	$(LD) $(LDFLAGS) $(INTERNALLDFLAGS) $(OBJ) -o $@

%.o: %.c
	@printf "CC\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

%.o: %.cpp
	@printf "CPP\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CPP) $(CPPFLAGS) $(INTERNALCPPFLAGS) -c $< -o $@

%_S.o: %.S
	@printf "AS\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CC) $(ASFLAGS) $(INTERNALASFLAGS) -c $< -o $@