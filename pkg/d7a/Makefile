PKG_NAME=d7a
PKG_URL=https://github.com/MOSAIC-LoPoW/dash7-ap-open-source-stack.git
#PKG_VERSION=
PKG_BUILDDIR ?= $(BINDIRBASE)/pkg/$(BOARD)/$(PKG_NAME)
PKG_DIR?=$(CURDIR)

$(info Compile D7A for DASH7 device)
#D7A_ARGS+= --enable-cli-app=ftd --enable-application-coap

$(info $$D7A_ARGS is [${D7A_ARGS}])

ifneq ($(wildcard $(PKG_BUILDDIR)),)
.PHONY: all
else 
.PHONY: all prepare git-download clean
endif

#D7A_COMMON_FLAGS = -fdata-sections -ffunction-sections -Os
#D7A_COMMON_FLAGS += -Wno-implicit-fallthrough

TOOLCHAIN_DIR ?=$(HOME)/cortus-ide/
TOOLCHAIN_FILE=stack/cmake/toolchains/$(CPU_ARCH)-gcc.cmake

# Add standard include directories
#INCLUDES += -I$(RIOTBASE)/core/include -I$(RIOTBASE)/drivers/include -I$(RIOTBASE)/sys/include
CFLAGS += -I $(PKG_DIR)/include -I$(RIOTBASE)/sys/include -I$(RIOTBASE)/core/include -I$(RIOTBASE)/drivers/include -I$(RIOTBOARD)/$(BOARD)/include -I$(RIOTCPU)/$(CPU)/include

# vfs dependency on statvfs (FIXME: todo in Makefile.include)
ifneq (,$(filter vfs,$(USEMODULE)))
	CFLAGS += -I$(RIOTBASE)/sys/posix/include
endif

#disable some warnings in the D7A stack
CUWFLAGS += -Wstrict-prototypes -Wold-style-definition -Werror
CFLAGS := $(filter-out $(CUWFLAGS), $(CFLAGS))
CFLAGS += -Wno-unused-function -Wno-unused-parameter -Wno-switch -Wno-missing-braces

all: $(PKG_BUILDDIR)/Makefile
	printenv && \
	"$(MAKE)" $(INCLUDES) -C $(PKG_BUILDDIR) && \
	cp $(PKG_BUILDDIR)/modules/modules/d7ap/libd7ap.a $(BINDIR)/$(PKG_NAME).a

$(PKG_BUILDDIR)/Makefile::
	cd "$(PKG_BUILDDIR)" && \
	cmake stack/ -DTOOLCHAIN_DIR=$(TOOLCHAIN_DIR) -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE) -DPLATFORM=$(BOARD)

prepare: git-download

GITAMFLAGS ?= --no-gpg-sign --ignore-whitespace

git-download:
#    $(info $$PKG_BUILDDIR is [${PKG_BUILDDIR}])
	rm -Rf $(PKG_BUILDDIR)
	mkdir -p $(PKG_BUILDDIR)
	git -C $(PKG_BUILDDIR) init
	git -C $(PKG_BUILDDIR) remote add -f origin $(PKG_URL)
	git -C $(PKG_BUILDDIR) config core.sparsecheckout true
	echo stack/cmake >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo stack/modules >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo stack/framework/inc >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo stack/framework/hal/inc >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo stack/CMakeLists.txt >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo !stack/modules/alp >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo !stack/modules/lorawan >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo !stack/framework/CMakeLists.txt >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo !stack/framework/components >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo !stack/framework/framework_bootstrap.c >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	git -C $(PKG_BUILDDIR) pull origin new_phy
	git -C $(PKG_BUILDDIR) am $(GITAMFLAGS) "$(PKG_DIR)"/patches/*.patch
	touch $@

clean::
	@rm -rf $(BINDIR)/$(PKG_NAME).a

distclean::
	rm -rf "$(PKG_BUILDDIR)"
