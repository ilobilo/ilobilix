ROOTDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
SOURCEDIR := $(ROOTDIR)/source
EXTLIBDIR := $(ROOTDIR)/extlibs
ISO_ROOT := $(ROOTDIR)/iso_root

INITRD ?= $(ISO_ROOT)/initrd.img.gz
ILARDIR ?= $(EXTLIBDIR)/ilar
ILAR ?= $(ILARDIR)/bin/ilar

TAR ?= tar

all: extlibs
	@$(MAKE) -s modules
	@$(MAKE) -s initrd
	@$(MAKE) -sC $(SOURCEDIR)

bios: extlibs
	@$(MAKE) -s modules
	@$(MAKE) -s initrd
	@$(MAKE) -sC $(SOURCEDIR) bios

.PHONY: extlibs
extlibs:
	@$(MAKE) -sC $(ROOTDIR)/extlibs

.PHONY: modules
modules:
	@$(MAKE) -sC $(ROOTDIR)/modules

modules-clean:
	@$(MAKE) -sC $(ROOTDIR)/modules clean

.PHONY: initrd
initrd:
	@mkdir -p $(ISO_ROOT)
ifndef USEILAR
	@printf "TAR\t%s\n" $(INITRD:$(ROOTDIR)/%=%)
	$(TAR) czf $(INITRD) -C $(ROOTDIR)/initrd/ ./ --format=ustar
else
	@printf "ILAR\t%s\n" $(INITRD:$(ROOTDIR)/%=%)
	$(ILAR) create $(INITRD) $(ROOTDIR)/initrd/*
endif

clean:
	@$(MAKE) -sC $(SOURCEDIR) clean
	@rm -f $(ROOTDIR)/log.txt

distclean: clean
	@$(MAKE) -sC $(ROOTDIR)/extlibs clean
	@$(MAKE) -sC $(ROOTDIR)/modules clean