APPLICATION = APPD7A_MODEM

# If no BOARD is found in the environment, use this default:
BOARD ?= cortus_fpga

BOARD_WHITELIST := cortus_fpga ciot25_demo_board

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 0 #1

USEPKG += d7a
USEMODULE += d7a_fs
USEMODULE += d7a_contrib
USEMODULE += xtimer

# use CORTUS XCVR driver by default
# You can also pass `DRIVER` when building the application:
# $ make BOARD=${RIOT_BOARD} DRIVER=sx1276
DRIVER ?= xcvr


#ifneq (,$(filter at86rf2%,$(DRIVER)))
  FEATURES_REQUIRED = periph_spi
  FEATURES_REQUIRED = periph_gpio
  FEATURES_REQUIRED = periph_uart
#endif

USEMODULE += $(DRIVER)

USEMODULE += random
#USEMODULE += shell
#USEMODULE += shell_commands
#USEMODULE += ps

#USEMODULE += dht
USEMODULE += fmt
USEMODULE += tsrb

# Use modules for networking
# gnrc is a meta module including all required, basic gnrc networking modules
#USEMODULE += gnrc
# use the default network interface for the board
#USEMODULE += gnrc_netdev_default
# automatically initialize the network interface
USEMODULE += auto_init_gnrc_netif

# the application dumps received packets to stdout
USEMODULE += gnrc_pktdump

#USEMODULE += ipv6_addr

#required for C++ compiling
#CXXEXFLAGS += -fno-rtti
#USEMODULE += cpp11-compat

#Define LOG enabled by default
CFLAGS += -DFRAMEWORK_LOG_ENABLED=yes
CFLAGS += -DPLATFORM_SX127X_USE_PA_BOOST=yes
CFLAGS += -Wno-missing-braces

#CFLAGS += -S -save-temps

#export RIOT_CLIB := newlib

# Filesystem: 
# export RIOT_D7A_FS=memarray|littlefs|spiffs
# export RIOT_D7A_MTD=dummy|m25p32
USEMODULE += mtd
#
# Note: memarray is the oss7 implementation without vfs and mtd.
# Note: vfs mounts and mtd devices must be defined and provided by the arch.
# Warning: littlefs on m25p32 reduces the number of access profiles due to lack of sectors.
#TODO: black/white list of compatible arches?
RIOT_D7A_FS ?= memarray
RIOT_D7A_MTD ?= dummy

ifneq (memarray,$(filter memarray, $(RIOT_D7A_FS)))
USEMODULE += vfs
CFLAGS += -DVFS_FILE_BUFFER_SIZE=52 -DVFS_DIR_BUFFER_SIZE=44
USEMODULE += tmpfs
ifneq (,$(filter littlefs, $(RIOT_D7A_FS)))
USEMODULE += mtd
USEMODULE += littlefs
else ifneq (,$(filter spiffs, $(RIOT_D7A_FS)))
USEMODULE += mtd
USEMODULE += spiffs
endif
ifneq (,$(filter m25p32, $(RIOT_D7A_MTD)))
USEMODULE += mtd_spi_nor
CFLAGS += -DUSE_PERIPH_CORTUS_MTD_FPGA_M25P32
endif
endif

#Application protocol
#export RIOT_D7A_AP=mqtt_sn|mqtt_sn_over_udp
ifneq (,$(filter mqtt_sn_over_udp, $(RIOT_D7A_AP)))
USEMODULE += gnrc_sock_udp
USEMODULE += gnrc_ipv6_default
USEMODULE += asymcute
else ifneq (,$(filter mqtt_sn, $(RIOT_D7A_AP)))
# Include MQTT-SN
USEMODULE += asymcute
endif

DEVELHELP ?= 1
#WERROR ?= 0

include $(RIOTBASE)/Makefile.include
