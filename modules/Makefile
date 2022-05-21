MODULEDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
INITRDDIR := $(MODULEDIR)/../initrd

ARCH ?= x86_64

ifneq ($(ARCH), $(filter $(ARCH), arm64 x86_64))
$(error Please set ARCH to either arm64 or x86_64)
endif

ifeq ($(ARCH), arm64)
MODSRCDIR := $(MODULEDIR)/arm64
else ifeq ($(ARCH), x86_64)
MODSRCDIR := $(MODULEDIR)/x86_64
endif

SUBDIRS := $(patsubst %/., %, $(realpath $(wildcard $(MODSRCDIR)/*/.)))

.PHONY: all
all: $(SUBDIRS)

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) --no-print-directory -C $@
	mv $(shell find $(MODSRCDIR) -type f -name '*.ko') $(INITRDDIR)/lib/modules

clean:
ifndef NOCLEAN
	rm -rf $(shell find $(INITRDDIR)/lib/modules/ -type f -name "*.ko")
endif