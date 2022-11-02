LDSCRIPT = $(KERNELDIR)/linker-$(ARCH).ld
ifeq ("$(wildcard $(LDSCRIPT))","")
    $(error Linker script $(LDSCRIPT) does not exist)
endif

KERNEL_CFLAGS ?= 
KERNEL_CXXFLAGS ?= 
KERNEL_ASFLAGS ?= 
KERNEL_LDFLAGS ?= 

CFLAGS += $(KERNEL_CFLAGS)
CXXFLAGS += $(KERNEL_CXXFLAGS)
ASFLAGS += $(KERNEL_ASLAGS)
LDFLAGS += $(KERNEL_LDLAGS)

override LDFLAGS +=                  \
	-T$(KERNELDIR)/linker-$(ARCH).ld \
	-nostdlib                        \
	-zmax-page-size=0x1000           \
	-static $(LIBRARIES)

override CCFLAGS :=               \
	$(INCLUDES) $(MACROS)         \
    -target $(TARGET)             \
	-ffreestanding                \
	-fno-stack-protector          \
	-fno-omit-frame-pointer       \
	-fno-pic -fno-pie             \
	-Werror -Wall -Wextra         \
	-Wno-error=\#warnings         \
	-Wno-format-invalid-specifier \
	-Ofast -pipe -MMD             \
	-DLVL5_PAGING=$(LVL5_PAGING)

ifeq ($(ARCH),x86_64)
    override CCFLAGS +=   \
        -march=x86-64     \
        -mabi=sysv        \
        -msoft-float      \
        -mno-mmx          \
        -mno-sse          \
        -mno-sse2         \
        -mno-red-zone     \
        -mcmodel=kernel # -mno-80387 -fasm-blocks

    override ASFLAGS += \
        -masm=intel
else ifeq ($(ARCH),aarch64)
    override CCFLAGS +=     \
        -mgeneral-regs-only \
        -mcmodel=small
endif

ifndef NOUBSAN
    override CCFLAGS += -fsanitize=undefined
endif

override CFLAGS +=  \
	$(CCFLAGS)      \
	-std=$(CVERSION)

override CXXFLAGS +=           \
	$(CCFLAGS)                 \
	-std=$(CXXVERSION)         \
	-fno-rtti                  \
	-fno-exceptions            \
	-fsized-deallocation       \
	-Wno-unused-parameter      \
	-Wno-deprecated-volatile   \
	-Wno-user-defined-literals

override ASFLAGS += \
	$(CCFLAGS)

override OTHERARCHDIRS := $(filter-out $(KERNELDIR)/arch/$(ARCH)/%,$(wildcard $(KERNELDIR)/arch/*/))
override OTHERARCHDIRS := $(if $(OTHERARCHDIRS),-not -path $(subst $(subst ,, ),\* -not -path ,$(OTHERARCHDIRS))\*,)

override CFILES := $(shell find $(KERNELDIR)/ -type f -name '*.c' $(OTHERARCHDIRS) 2> /dev/null) \
	$(shell find $(EXTDEPDIR)/limine-terminal-port/source/ -type f -name '*.c' 2> /dev/null)     \
	$(shell find $(EXTDEPDIR)/lai/ -type f -name '*.c' 2> /dev/null)                             \
	$(EXTDEPDIR)/cwalk/src/cwalk.c $(EXTDEPDIR)/printf/src/printf/printf.c

override CXXFILES := $(shell find $(KERNELDIR)/ -type f -name '*.cpp' $(OTHERARCHDIRS) 2> /dev/null)
override ASFILES := $(shell find $(KERNELDIR)/ -type f -name '*.S' $(OTHERARCHDIRS) 2> /dev/null)

override OBJ := $(CFILES:.c=.c.o) $(CXXFILES:.cpp=.cpp.o) $(ASFILES:.S=.S.o)
override HDP := $(CFILES:.c=.c.d) $(CXXFILES:.cpp=.cpp.d) $(ASFILES:.S=.S.d)

.PHONY: all
all: $(KERNEL)

$(KERNEL): $(OBJ)
	@printf "LD\t%s\n" $(KERNEL:$(ROOTDIR)/%=%)
	$(LD) $(LDFLAGS) $(OBJ) $(KERNEL_MODULES) -o $@

-include $(HDP)

%.c.o: %.c
	@printf "CC\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CC) $(CFLAGS) -D__FILENAME__='"$(<:$(ROOTDIR)/%=%)"' -c $< -o $@

%.cpp.o: %.cpp
	@printf "CXX\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CXX) $(CXXFLAGS) -D__FILENAME__='"$(<:$(ROOTDIR)/%=%)"' -c $< -o $@

%.S.o: %.S
	@printf "AS\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CC) $(ASFLAGS) -D__FILENAME__='"$(<:$(ROOTDIR)/%=%)"' -c $< -o $@

.PHONY: clean
clean:
ifndef NOCLEAN
	rm -f $(OBJ) $(HDP) $(KERNEL)
endif
