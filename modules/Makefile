.PHONY: all
all: $(MODULES)

$(MODULES):
	$(MAKE) -f$(MODULEDIR)/Makefile.in -C $(shell dirname $@)

.PHONY: clean
clean:
ifndef NOCLEAN
	rm -rf $(MODULES)
endif