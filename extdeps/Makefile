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

override DEPS :=                              \
	$(EXTDEPDIR)/lai $(EXTDEPDIR)/frigg       \
	$(EXTDEPDIR)/cwalk $(EXTDEPDIR)/printf    \
	$(EXTDEPDIR)/fmt                          \
	$(EXTDEPDIR)/smart_ptr $(EXTDEPDIR)/veque \
	$(EXTDEPDIR)/libstdcxx-headers            \
	$(EXTDEPDIR)/libgcc-binaries              \
	$(EXTDEPDIR)/limine-terminal-port         \
	$(EXTDEPDIR)/limine $(OVMFDIR)

.PHONY: all
all: $(DEPS)

$(EXTDEPDIR)/lai:
	$(call git_clone,managarm,lai)

$(EXTDEPDIR)/frigg:
	$(call git_clone,managarm,frigg)

$(EXTDEPDIR)/cwalk:
	$(call git_clone,likle,cwalk)

$(EXTDEPDIR)/printf:
	$(call git_clone,eyalroz,printf)

$(EXTDEPDIR)/fmt:
	$(call git_clone,fmtlib/fmt)

$(EXTDEPDIR)/smart_ptr:
	$(call git_clone,ilobilo,smart_ptr)

$(EXTDEPDIR)/veque:
	$(call git_clone,ilobilo,veque)

$(EXTDEPDIR)/libstdcxx-headers:
	$(call git_clone,ilobilo,libstdcxx-headers)

$(EXTDEPDIR)/libgcc-binaries:
	$(call git_clone,mintsuki,libgcc-binaries,trunk)

$(EXTDEPDIR)/limine-terminal-port:
	$(call git_clone,V01D-NULL,limine-terminal-port)

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
	rm -rf $(DEPS) ovmf-*