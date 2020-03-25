PKG_NAME=d7a
PKG_URL=git@gitlab.cortus.com:CIoT_SDK/dash7-ap-open-source-stack.git
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

ifeq ($(CPU_ARCH),cortex-m0plus)
TOOLCHAIN_FILE=stack/cmake/toolchains/gcc-arm-embedded.cmake
else
TOOLCHAIN_FILE=stack/cmake/toolchains/$(CPU_ARCH)-gcc.cmake
endif

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
	"$(MAKE)" -C $(PKG_BUILDDIR) && \
	cp $(PKG_BUILDDIR)/modules/modules/d7ap/libd7ap.a $(BINDIR)/$(PKG_NAME).a
	cp $(PKG_BUILDDIR)/modules/modules/d7ap_fs/libd7ap_fs.a $(BINDIR)/$(PKG_NAME)_fs.a

$(PKG_BUILDDIR)/Makefile::
	cd "$(PKG_BUILDDIR)" && \
	cmake stack/ -DFRAMEWORK_LOG_ENABLED=TRUE -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE) -DEXTERNAL_C_FLAGS="$(INCLUDES)"

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
	echo stack/fs >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo stack/modules >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo stack/framework/inc >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo stack/framework/hal/inc >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo stack/CMakeLists.txt >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo !stack/modules/alp >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo !stack/modules/lorawan >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo !stack/framework/CMakeLists.txt >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo !stack/framework/components >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	echo !stack/framework/framework_bootstrap.c >> $(PKG_BUILDDIR)/.git/info/sparse-checkout
	git -C $(PKG_BUILDDIR) pull origin demo
	git -C $(PKG_BUILDDIR) am $(GITAMFLAGS) "$(PKG_DIR)"/patches/*.patch
	touch $@

clean::
	@rm -rf $(BINDIR)/$(PKG_NAME).a

distclean::
	rm -rf "$(PKG_BUILDDIR)"
