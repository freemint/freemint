#
# Makefile for minixfs tools
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = tools

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

include $(srcdir)/../MINIXDEFS

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(alltargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

ALL_TARGETS = $(foreach TARGET,$(alltargets),.compile_$(TARGET)/flist .compile_$(TARGET)/mfsconf)

strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

define TARGET_TEMPLATE

$(1): .compile_$(1)/flist .compile_$(1)/mfsconf

LIBS_$(1) =
OBJS_flist_$(1) = $(foreach OBJ, flist, .compile_$(1)/$(OBJ).o)
OBJS_mfsconf_$(1) = $(foreach OBJ, mfsconf, .compile_$(1)/$(OBJ).o)
DEFINITIONS_$(1) = $(DEFINITIONS)

.compile_$(1)/flist: $$(OBJS_flist_$(1))
	$(LD) $$(LDEXTRA_$(1)) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_flist_$(1)) $$(LIBS_$(1))

.compile_$(1)/mfsconf: $$(OBJS_mfsconf_$(1))
	$(LD) $$(LDEXTRA_$(1)) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_mfsconf_$(1)) $$(LIBS_$(1))

endef

$(foreach TARGET,$(alltargets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(alltargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(alltargets))) > /dev/null 2>&1 || :)
endif
