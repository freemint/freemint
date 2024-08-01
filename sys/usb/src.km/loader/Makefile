#
# Makefile for USB loader
#
SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../..
subdir = loader
toscommon_srcdir = $(srcdir)/../tos-common

default: help

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(srcdir)/USBLOADDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites
INCLUDES = 
DEFINITIONS = -DSTACKSIZE=2048

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)

# Those were generated/linked here by older version of this Makefile
# only keep this rule to ease transition
GENFILES += crtinit.c entry.S setstack.S

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(usbloadtargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

ALL_TARGETS = $(foreach TARGET, $(usbloadtargets), .compile_$(TARGET)/loader.prg)

STRIPFLAGS =
strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

LDEXTRA = -nostdlib -Wl,--entry -Wl,__start

define TARGET_TEMPLATE

LIBKERN_NAME_$(1) = kern$$(CPU_$(1))
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS) $$(USBLOADDEFS_$(1))
LIBS_$(1) = $$(LIBKERN_$(1)) -lgcc
OBJS_$(1) = $(foreach OBJ, $(notdir $(basename $(STARTUP_$(1)) $(COBJS))), .compile_$(1)/$(OBJ).o)

$(1): $(addprefix .compile_,$(1))/loader.prg

$(addprefix .compile_,$(1))/loader.prg: $$(OBJS_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(1)) $$(LIBS_$(1))

endef

define LIBKERN_TEMPLATE
$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern libkern$(1).a
endef

$(foreach TARGET,$(usbloadtargets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(usbloadtargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(usbloadtargets))) > /dev/null 2>&1 || :)
endif
