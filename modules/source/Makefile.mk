THISDIR := $(realpath $(shell pwd))
MODULE := $(THISDIR)/$(notdir $(THISDIR)).ko

MODULE_CFLAGS ?= 
MODULE_CXXFLAGS ?= 
MODULE_ASFLAGS ?= 
MODULE_LDFLAGS ?= 

CFLAGS += $(MODULE_CFLAGS)
CXXFLAGS += $(MODULE_CXXFLAGS)
ASFLAGS += $(MODULE_ASFLAGS)
LDFLAGS += $(MODULE_LDLAGS)

-include Makefile.mk

override LDFLAGS += \
	-relocatable    \
	-nostdlib       \
	-static

override CCFLAGS :=         \
	$(INCLUDES)             \
	-ffreestanding          \
	-fno-stack-protector    \
	-fno-omit-frame-pointer \
	-fno-pic                \
	-fno-pie                \
	-Werror -Wall -Wextra   \
	-Wno-error=\#warnings   \
	-Ofast -pipe -MMD       \

ifeq ($(ARCH),x86_64)
    override CCFLAGS +=   \
        -target $(TARGET) \
        -march=x86-64     \
        -mabi=sysv        \
        -mno-80387        \
        -mno-mmx          \
        -mno-sse          \
        -mno-sse2         \
		-mno-red-zone     \
        -mcmodel=large

    override ASFLAGS += \
        -masm=intel
else
endif

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

override ASFLAGS += \
	$(CCFLAGS)

override CFILES := $(shell find $(THISDIR)/ -type f -name '*.c')
override CXXFILES := $(shell find $(THISDIR)/ -type f -name '*.cpp')
override ASFILES := $(shell find $(THISDIR)/ -type f -name '*.S')

override OBJ = $(CFILES:.c=.c.o) $(CXXFILES:.cpp=.cpp.o) $(ASFILES:.S=.S.o)
override HDP = $(CFILES:.c=.c.d) $(CXXFILES:.cpp=.cpp.d) $(ASFILES:.S=.S.d)

.PHONY: all
all: $(MODULE)
ifndef NOCLEAN
	rm -f $(OBJ) $(HDP)
endif

$(MODULE): $(OBJ)
	@printf "LD\t%s\n" $(MODULE:$(ROOTDIR)/%=%)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

-include $(HDP)

%.c.o: %.c
	@printf "CC\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CC) $(CFLAGS) -c $< -o $@

%.cpp.o: %.cpp
	@printf "CXX\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.S.o: %.S
	@printf "AS\t%s\n" $(<:$(ROOTDIR)/%=%)
	$(CC) $(ASFLAGS) -c $< -o $@