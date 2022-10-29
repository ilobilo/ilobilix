define err
    echo "\e[31mFailed to download $(1)!\e[0m"
	$(error)
endef

define git_clone
	echo "Downloading $(2)"
	git clone --single-branch --branch=$(if $(3),$(3),master) --depth=2 https://github.com/$(1)/$(2) || $(call err,$(2))
endef

define wget_down
	echo "Downloading $(1)"
	wget $(3) -O $(2) $(4) || $(call err,$(1))
endef

override DEPS :=                                \
	$(EXTDEPDIR)/lai $(EXTDEPDIR)/cxxshim       \
	$(EXTDEPDIR)/frigg $(EXTDEPDIR)/span-lite   \
	$(EXTDEPDIR)/smart_ptr $(EXTDEPDIR)/veque   \
	$(EXTDEPDIR)/cwalk $(EXTDEPDIR)/printf      \
	$(EXTDEPDIR)/limine-terminal-port           \
	$(EXTDEPDIR)/libgcc-binaries                \
	$(EXTDEPDIR)/limine $(OVMFDIR)

.PHONY: all
all: $(DEPS)

$(EXTDEPDIR)/lai:
	$(call git_clone,managarm,lai)

$(EXTDEPDIR)/cxxshim:
	$(call git_clone,ilobilo,cxxshim)

$(EXTDEPDIR)/frigg:
	$(call git_clone,ilobilo,frigg)

$(EXTDEPDIR)/span-lite:
	$(call git_clone,martinmoene,span-lite)

$(EXTDEPDIR)/smart_ptr:
	$(call git_clone,ilobilo,smart_ptr)

$(EXTDEPDIR)/veque:
	$(call git_clone,ilobilo,veque)

$(EXTDEPDIR)/cwalk:
	$(call git_clone,likle,cwalk)

$(EXTDEPDIR)/printf:
	$(call git_clone,eyalroz,printf)

$(EXTDEPDIR)/limine-terminal-port:
	$(call git_clone,V01D-NULL,limine-terminal-port)

$(EXTDEPDIR)/libgcc-binaries:
	$(call git_clone,mintsuki,libgcc-binaries,trunk)

$(EXTDEPDIR)/limine:
	$(call git_clone,limine-bootloader,limine,v4.x-branch-binary)
	$(MAKE) -sC $(EXTDEPDIR)/limine

override OVMF_ZIP := $(EXTDEPDIR)/OVMF-$(EFI_ARCH).zip
$(OVMFDIR):
	$(call wget_down,ovmf,$(OVMF_ZIP),https://efi.akeo.ie/OVMF/OVMF-$(EFI_ARCH).zip)
	unzip $(OVMF_ZIP) -d $(OVMFDIR)
	rm -f $(OVMF_ZIP)

.PHONY: clean
clean:
	rm -rf $(DEPS)