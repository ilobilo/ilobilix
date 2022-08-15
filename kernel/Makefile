LDSCRIPT = $(KERNELDIR)/linker-$(ARCH).ld
ifeq ("$(wildcard $(LDSCRIPT))","")
    $(error Linker script $(LDSCRIPT) does not exist)
endif

KERNEL_CFLAGS ?= -g
KERNEL_CXXFLAGS ?= $(KERNEL_CFLAGS)
KERNEL_LDFLAGS ?= 
KERNEL_ASFLAGS ?= $(KERNEL_CFLAGS)

CFLAGS += $(KERNEL_CFLAGS)
CXXFLAGS += $(KERNEL_CXXFLAGS)
LDFLAGS += $(KERNEL_LDLAGS)
ASFLAGS += $(KERNEL_ASLAGS)

override CCFLAGS :=              \
	-ffreestanding               \
	-fno-stack-protector         \
	-fno-omit-frame-pointer      \
	-fsized-deallocation         \
	-DNET_DEBUG=$(NET_DEBUG)     \
	-DLVL5_PAGING=$(LVL5_PAGING)

override LDFLAGS +=                  \
	-T$(KERNELDIR)/linker-$(ARCH).ld \
	-nostdlib                        \
	-zmax-page-size=0x1000           \
	-static $(GLOBALLIBS)

override ASFLAGS +=     \
	-mgeneral-regs-only

override ARCHFLAGS := \
	$(INCLUDES)       \
	-fno-pie          \
	-fno-pic

ifeq ($(ARCH),x86_64)
    override ARCHFLAGS +=          \
		-target x86_64-pc-none-elf \
		-march=x86-64              \
		-mabi=sysv                 \
		-mno-80387                 \
		-mno-mmx                   \
		-mno-sse                   \
		-mno-sse2                  \
		-mcmodel=kernel

    override CCFLAGS += \
		-mno-red-zone

    override ASFLAGS += \
		-masm=intel
else
endif

override ASFLAGS += $(ARCHFLAGS)
override CCFLAGS += $(ARCHFLAGS)

ifndef NOUBSAN
    override CCFLAGS += -fsanitize=undefined
endif

override CFLAGS += \
	$(CCFLAGS)     \
	-std=$(CVERSION)

override CXXFLAGS +=           \
	$(CCFLAGS)                 \
	-std=$(CXXVERSION)         \
	-fno-rtti                  \
	-fno-exceptions            \
	-Wno-register              \
	-Wno-c99-designator        \
	-Wno-unused-parameter      \
	-Wno-deprecated-volatile   \
	-Wno-user-defined-literals

override OTHERARCHDIRS := $(filter-out $(KERNELDIR)/arch/$(ARCH)/%,$(wildcard $(KERNELDIR)/arch/*/))
override OTHERARCHDIRS := $(if $(OTHERARCHDIRS),-not -path $(subst $(subst ,, ),\* -not -path ,$(OTHERARCHDIRS))\*,)

override CFILES := $(shell find $(KERNELDIR)/ -type f -name '*.c' $(OTHERARCHDIRS) 2> /dev/null) \
	$(shell find $(EXTDEPDIR)/limine-terminal-port/source/ -type f -name '*.c' 2> /dev/null)     \
	$(shell find $(EXTDEPDIR)/cwalk/src/ -type f -name '*.c' 2> /dev/null)                       \
	$(shell find $(EXTDEPDIR)/lai/ -type f -name '*.c' 2> /dev/null)

override CXXFILES := $(shell find $(KERNELDIR)/ -type f -name '*.cpp' $(OTHERARCHDIRS) 2> /dev/null)
override ASFILES := $(shell find $(KERNELDIR)/ -type f -name '*.S' $(OTHERARCHDIRS) 2> /dev/null)

override OBJ = $(CFILES:.c=.o)
override OBJ += $(CXXFILES:.cpp=.o)
override OBJ += $(ASFILES:.S=_S.o)

.PHONY: all
all: $(KERNEL)

$(KERNEL): $(OBJ)
	@printf "LD\t%s\n" $(KERNEL:$(ROOTDIR)/%=%)
	$(LD) $(LDFLAGS) $(OBJ) $(KERNEL_MODULES) -o $@

%.o: %.c
	@printf "CC\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	@printf "CXX\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%_S.o: %.S
	@printf "AS\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CC) $(ASFLAGS) -c $< -o $@

.PHONY: clean
clean:
ifndef NOCLEAN
	rm -f $(OBJ) $(KERNEL)
endif