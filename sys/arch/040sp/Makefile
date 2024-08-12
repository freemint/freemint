#
# the m68k 040sp
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = 040sp

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

# default overwrites
LDEXTRA = -nostdlib -Wl,--entry,_init
GENERAL += -Wa,--defsym,GAS=1
INCLUDES += -I$(srcdir)/src

alltargets = 040

ALL_TARGETS = 040sp.prg

all-here: $(ALL_TARGETS)

# default definitions
compile_all_dirs = .compile_* 
GENFILES = $(compile_all_dirs) *.prg

OBJS = $(foreach OBJ, $(notdir $(basename $(COBJS) $(FPSP040_SRCS))), .compile_040/$(OBJ).o)

strip:
	$(STRIP) $(ALL_TARGETS)

040sp.prg: $(OBJS)
	$(LD) $(CFLAGS_040) -o $@ $(OBJS)

$(foreach TARGET,$(alltargets),$(foreach OBJ,$(COBJS) $(FPSP040_SRCS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(foreach DIR, $(alltargets), .compile_$(DIR)/.deps) > /dev/null 2>&1 || :)
endif
