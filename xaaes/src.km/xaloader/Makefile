#
# Makefile for XaAES loader
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = xaloader

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

include $(srcdir)/XALOADDEFS

all-here: all-targets

# default overwrites
LDEXTRA = -nostdlib -Wl,--entry,__start -Wl,--fatal-warnings
LIBS = -lgcc

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(xaloadtargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

ALL_TARGETS = $(foreach TARGET,$(xaloadtargets),.compile_$(TARGET)/xaloader.prg)

strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

define TARGET_TEMPLATE

DEFINITIONS_$(1) = $(DEFINITIONS) $$(XAAESDEFS_$(1))
LIBS_$(1) =
OBJS_$(1) = $(foreach OBJ, $(notdir $(basename $(COBJS))), .compile_$(1)/$(OBJ).o)

$(1): .compile_$(1)/xaloader.prg

.compile_$(1)/xaloader.prg: $$(OBJS_$(1))
	$(LD) $$(CFLAGS_$(CPU_$(1))) -o $$@ $$(OBJS_$(1)) $(LIBS) $$(LIBS_$(1))

.compile_$(1)/info.o .compile_$(1)/about.o: xversion.h

endef

$(foreach TARGET,$(xaloadtargets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(xaloadtargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))


ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(xaloadtargets))) > /dev/null 2>&1 || :)
endif
