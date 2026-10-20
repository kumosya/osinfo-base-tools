INFOIDE ?= $(CURDIR)/infoide
PROJECTS := coreutils sysinfo mount mkfs systemctl
PACKAGES := $(foreach project,$(PROJECTS),$(project)/build/packages/$(project).inpk)
.PHONY: all package $(PROJECTS)
all package: build/osinfo-base-tools.zip
$(PROJECTS):
	$(MAKE) -C "$@" INFOIDE="$(abspath $(INFOIDE))" $(if $(OSINFO_SDK),OSINFO_SDK="$(abspath $(OSINFO_SDK))")
build/osinfo-base-tools.zip: $(PROJECTS)
	@mkdir -p build
	"$(INFOIDE)" bundle --output "$@" $(PACKAGES)
