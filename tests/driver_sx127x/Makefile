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
DRIVER ?= sx1276

# use SX1276 by default
USEMODULE += $(DRIVER)

FEATURES_REQUIRED ?= periph_spi
FEATURES_REQUIRED ?= periph_gpio

export RIOT_CLIB := newlib

include $(RIOTBASE)/Makefile.include
