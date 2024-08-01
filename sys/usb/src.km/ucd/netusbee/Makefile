#
# Makefile for netusbee
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../../../..
subdir = netusbee
toscommon_srcdir = $(srcdir)/../../tos-common

default: help

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

include $(srcdir)/NETUSBEEDEFS
include $(srcdir)/../../STRIPEXDEFS

all-here: all-targets

# default overwrites
DEFINITIONS = -D__KERNEL_MODULE__ -DMODULE_NAME=netusbee $(USBDEFS)
WARN += -Wcast-qual

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) *.ucd *.prg

# Those were generated/linked here by older version of this Makefile
# only keep this rule to ease transition
GENFILES += crtinit.c delayasm.S entry.S setstack.S

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(netusbeetargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

ALL_TARGETS = $(foreach TARGET, $(netusbeetargets), .compile_$(TARGET)/$(UCDTARGET_$(TARGET)))

STRIPFLAGS =
strip:
	$(STRIP) $(filter-out %.prg,$(ALL_TARGETS))

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

# for HDDRIVER modules to work, TOSSCOBJS must be first in OBJS
OBJS_prg = $(foreach OBJ, $(notdir $(basename $(TOSCOBJS))), .compile_prg/$(OBJ).o)
OBJS_prg_000 = $(foreach OBJ, $(notdir $(basename $(TOSCOBJS))), .compile_prg_000/$(OBJ).o)

define TARGET_TEMPLATE

LIBKERN_NAME_$(1) = kern$$(CPU_$(1))
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS) $$(USBDEFS_$(1))
LIBS_$(1) = $$(LIBKERN_$(1)) -lgcc
OBJS_$(1) += $(foreach OBJ, $(notdir $(basename $(COBJS))), .compile_$(1)/$(OBJ).o)
$$(OBJS_$(1)): NETUSBEEDEFS

$(1): $(addprefix .compile_,$(1))/$(UCDTARGET_$(1))

$(addprefix .compile_,$(1))/$(UCDTARGET_$(1)): $$(OBJS_$(1)) $$(LIBKERN_DEP_$(1)) $(STRIPEX)
	$(LD) $$(LDEXTRA_$(1)) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(1)) $$(LIBS_$(1))
	@case $$@ in *.prg) echo $(STRIPEX) $$@; $(STRIPEX) $$@; ;; esac

endef

define LIBKERN_TEMPLATE
$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern libkern$(1).a
endef

$(foreach TARGET,$(netusbeetargets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(netusbeetargets),$(foreach OBJ,$(COBJS) $(TOSCOBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(netusbeetargets))) > /dev/null 2>&1 || :)
endif
