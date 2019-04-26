# Target architecture for the build.
export TARGET_ARCH ?= aps
#set default for CPU_MODEL
export MCPU ?= aps3r

# define build specific options
CFLAGS_CPU   = -mcpu=$(MCPU) -std=gnu99 -fno-common
CFLAGS_LINK  = -ffunction-sections -fdata-sections -static
CFLAGS_DBG  ?= -g3
#CFLAGS_DBG  += -v -Wl,--verbose
CFLAGS_OPT  ?= -Os

# Explicit Library selection: 
#  miniclib by default, but lacks some unistd.h definitions
#  newlib highly increases the bin size
# FIXME: miniclib: "porting not complete" (youssef 20180521) 
#                   headers not inlined with exported symbols (i.e. atoi...)
#
#CFLAGS_CPU += -fuse-clib=mini
# aps-size /home/jmhemstedt/proj/d7/sw/riot/tests/periph_uart/bin/cortus_fpga/tests_periph_uart.elf
#   text	   data	    bss	    dec	    hex	filename
#   8524	    112	  24392	  33028	   8104	
#CFLAGS_CPU += -fuse-clib=newlib
# aps-size /home/jmhemstedt/proj/d7/sw/riot/tests/periph_uart/bin/cortus_fpga/tests_periph_uart.elf
#   text	   data	    bss	    dec	    hex	filename
#  32664	   2308	  24444	  59416	   e818

# 'export RIOT_CLIB=newlib' to override, mini by default.
RIOT_CLIB ?= mini 
CFLAGS_CPU += -fuse-clib=$(RIOT_CLIB)
ifneq (,$(filter mini,$(RIOT_CLIB)))
  CFLAGS_CPU += -DAPS_MINICLIB=1
else
  ifneq (,$(filter vfs,$(USEMODULE)))
#FIXME: aps newlib multiple definitions errors
  	USEMODULE += newlib
  	USEMODULE += newlib_syscalls_default
  endif
endif



# export compiler flags
export CFLAGS += $(CFLAGS_CPU) $(CFLAGS_LINK) $(CFLAGS_DBG) $(CFLAGS_OPT)
export CFLAGS += -Wall -MMD -MP -fmessage-length=0
# export assembly flags
export ASFLAGS += $(CFLAGS_CPU) $(CFLAGS_DBG)
# export linker flags
export LINKFLAGS += -L$(RIOTCPU)/$(CPU)/ldscripts
#export LINKER_SCRIPT ?= $(CPU_MODEL).ld
export LINKFLAGS += -T$(LINKER_SCRIPT) -Wl,--fatal-warnings
export LINKFLAGS += $(CFLAGS_CPU) $(CFLAGS_DBG) $(CFLAGS_OPT)
export LINKFLAGS += -fno-lto -Wl,-Map,cortus.map
#export LINKFLAGS += -fno-exceptions
#export LINKFLAGS += -Wl,-q 
export LINKFLAGS += -Wl,--noinhibit-exec
#export LINKFLAGS += -Wl,--whole-archive

## set -Wl,--whole-archive flag to have kernel weaks (core.a) resolved by globals in arch (cpu.a)
## requires a pacthed RIOT/Makefile.include, because it must be enabled only in --start-group.
export LINK_WHOLE_ARCHIVE =1

# overload the default crt0.o by the one provided by cpu/
export LINKFLAGS += -nostartfiles

# disable error on warning
#export WERROR = 0

