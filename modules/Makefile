.PHONY: all
all: $(MODULES)

$(MODULES):
	$(MAKE) -f$(MODULEDIR)/Makefile.mk -C $(shell dirname $@)

.PHONY: clean
clean:
ifndef NOCLEAN
	rm -rf $(MODULES)
endif