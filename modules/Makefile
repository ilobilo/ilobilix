MODULEDIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
INITRDDIR := $(realpath $(MODULEDIR)/../initrd)

MODSRCDIR := $(realpath $(MODULEDIR)/src/)
MODDESTDIR := $(realpath $(INITRDDIR)/lib/modules)

SUBDIRS := $(patsubst %/., %, $(realpath $(wildcard $(MODSRCDIR)/*/.)))

.PHONY: all
all: $(SUBDIRS)

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) --no-print-directory -f$(MODSRCDIR)/Makefile -C $@

clean:
ifndef NOCLEAN
	rm -rf $(shell find $(MODDESTDIR) -type f -name "*.ko")
endif