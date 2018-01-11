# Target architecture for the build.
export TARGET_ARCH ?= aps
export MCPU := aps3r

# define build specific options
CFLAGS_CPU   = -mcpu=$(MCPU) -std=gnu99 -fno-common
CFLAGS_LINK  = -ffunction-sections -fdata-sections
CFLAGS_DBG  ?= -g3
CFLAGS_OPT  ?= -Os

# export compiler flags
export CFLAGS += $(CFLAGS_CPU) $(CFLAGS_LINK) $(CFLAGS_DBG) $(CFLAGS_OPT)
export CFLAGS += -Wall -MMD -MP -fmessage-length=0
# export assmebly flags
export ASFLAGS += $(CFLAGS_CPU) $(CFLAGS_DBG)
# export linker flags
export LINKFLAGS += -L$(RIOTCPU)/$(CPU)/ldscripts
#export LINKER_SCRIPT ?= $(CPU_MODEL).ld
export LINKFLAGS += -T$(LINKER_SCRIPT) -Wl,--fatal-warnings
export LINKFLAGS += $(CFLAGS_CPU) $(CFLAGS_DBG) $(CFLAGS_OPT)
export LINKFLAGS += -fno-lto -Wl,-q -Wl,--noinhibit-exec -Wl,-Map,cortus.map -fno-exceptions

