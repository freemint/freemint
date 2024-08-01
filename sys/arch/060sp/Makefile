#
# the m68k 060sp
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = 060sp

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

# default overwrites
LDEXTRA = -nostdlib -Wl,--entry,_init

alltargets = 060

ALL_TARGETS = 060sp.prg

all-here: $(ALL_TARGETS)

# default definitions
compile_all_dirs = .compile_* 
GENFILES = $(compile_all_dirs) fpsp.S isp.S *.prg

OBJS = $(foreach OBJ, $(notdir $(basename $(COBJS))), .compile_060/$(OBJ).o)

strip:
	$(STRIP) $(ALL_TARGETS)

060sp.prg: $(OBJS)
	$(LD) $(CFLAGS_060) -o $@ $(OBJS) $(LDEXTRA)

.compile_060/mint060sp.o: fpsp.S isp.S

fpsp.S: motorola/fpsp.sa
	$(SHELL) asm2gas $< > $@

isp.S: motorola/isp.sa
	$(SHELL) asm2gas $< > $@

$(foreach TARGET,$(alltargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(foreach DIR, $(alltargets), .compile_$(DIR)/.deps) > /dev/null 2>&1 || :)
endif
