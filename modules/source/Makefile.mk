THISDIR := $(realpath $(shell pwd))
MODULE := $(THISDIR)/$(notdir $(THISDIR)).ko

MODULE_CFLAGS ?= 
MODULE_CXXFLAGS ?= 
MODULE_LDFLAGS ?= 
MODULE_ASFLAGS ?= 

CFLAGS += $(MODULE_CFLAGS)
CXXFLAGS += $(MODULE_CXXFLAGS)
LDFLAGS += $(MODULE_LDLAGS)
ASFLAGS += $(MODULE_ASLAGS)

-include Makefile.mk

override CCFLAGS :=      \
	-ffreestanding       \
	-fno-stack-protector

override LDFLAGS += \
	-relocatable    \
	-nostdlib       \
	-static         \

override ASFLAGS +=     \
	-mgeneral-regs-only

override ARCHFLAGS := \
	$(INCLUDES)       \
	-fno-pic          \
	-fno-pie

ifeq ($(ARCH),x86_64)
    override ARCHFLAGS +=          \
		-target x86_64-pc-none-elf \
		-march=x86-64              \
		-mabi=sysv                 \
		-mno-80387                 \
		-mno-mmx                   \
		-mno-sse                   \
		-mno-sse2                  \
		-mcmodel=large

    override CCFLAGS += \
		-mno-red-zone

    override ASFLAGS += \
		-masm=intel
else
endif

override ASFLAGS += $(ARCHFLAGS)
override CCFLAGS += $(ARCHFLAGS)

ifdef MODUBSAN
    override CCFLAGS += -fsanitize=undefined
endif

override CFLAGS += \
	$(CCFLAGS)     \
	-std=$(CVERSION)

override CXXFLAGS +=   \
	$(CCFLAGS)         \
	-std=$(CXXVERSION) \
	-fno-exceptions    \
	-fno-rtti

override CFILES := $(shell find $(THISDIR)/ -type f -name '*.c')
override CXXFILES := $(shell find $(THISDIR)/ -type f -name '*.cpp')
override ASFILES := $(shell find $(THISDIR)/ -type f -name '*.S')

override OBJ = $(CFILES:.c=.o)
override OBJ += $(CXXFILES:.cpp=.o)
override OBJ += $(ASFILES:.S=_S.o)

.PHONY: all
all: $(MODULE)
ifndef NOCLEAN
	rm -f $(OBJ)
endif

$(MODULE): $(OBJ)
	@printf "LD\t%s\n" $(MODULE:$(ROOTDIR)/%=%)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

%.o: %.c
	@printf "CC\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	@printf "CXX\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%_S.o: %.S
	@printf "AS\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CC) $(ASFLAGS) -c $< -o $@