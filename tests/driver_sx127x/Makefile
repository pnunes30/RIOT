BOARD ?= b-l072z-lrwan1

include ../Makefile.tests_common

BOARD_INSUFFICIENT_MEMORY := arduino-duemilanove arduino-leonardo arduino-nano\
                             arduino-uno nucleo-f031k6

USEMODULE += od
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += xtimer

USEPKG += d7a
USEMODULE += d7a_contrib
USEMODULE += mtd
DRIVER ?= sx1276

# use SX1276 by default
USEMODULE += $(DRIVER)

FEATURES_REQUIRED += periph_spi
FEATURES_REQUIRED += periph_gpio

ifeq ($(BOARD),b-l072z-lrwan1)
FEATURES_REQUIRED += periph_eeprom
endif

export RIOT_CLIB := newlib
CFLAGS += -Wno-missing-braces

include $(RIOTBASE)/Makefile.include
