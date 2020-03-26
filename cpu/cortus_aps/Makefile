# thread_arch.c's inline assembler breaks when compiling with link time
# optimization. see https://github.com/RIOT-OS/RIOT/issues/5774.
#SRC_NOLTO += thread_arch.c

# define the module that is build
MODULE = cpu
# add a list of subdirectories, that should also be build

ifeq (cortus_fpga,$(BOARD))
DIRS = periph
else
DIRS = periph xcvr
endif

include $(RIOTBASE)/Makefile.base
