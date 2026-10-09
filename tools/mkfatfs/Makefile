#
# Makefile for mkfatfs and gemkfatfs
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = mkfatfs

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

include $(srcdir)/MKFATFSDEFS

all-here: all-targets

# default overwrites
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/libIO
OPTS += -Wno-inline

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)
LIBIO_COBJS = $(top_srcdir)/libIO/xhdi.c

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

ALL_TARGETS = $(foreach TARGET,$(alltargets),.compile_$(TARGET)/mkfatfs .compile_$(TARGET)/gemkfatfs.app)

strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

define TARGET_TEMPLATE

$(1): .compile_$(1)/mkfatfs .compile_$(1)/gemkfatfs.app

LIBS_$(1) =
OBJS_mkfatfs_$(1) = $(foreach OBJ, mkfatfs xhdi, .compile_$(1)/$(OBJ).o)
OBJS_gemkfatfs_$(1) = $(foreach OBJ, startup gemkfatfs util, .compile_$(1)/$(OBJ).o)
DEFINITIONS_$(1) = $(DEFINITIONS)

.compile_$(1)/mkfatfs: $$(OBJS_mkfatfs_$(1))
	$(LD) $$(LDEXTRA_$(1)) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_mkfatfs_$(1)) $$(LIBS_$(1))

.compile_$(1)/gemkfatfs.app: $$(OBJS_gemkfatfs_$(1))
	$(LD) $$(LDEXTRA_$(1)) -nostartfiles -nostdlib -Wl,--entry,__start -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_gemkfatfs_$(1)) $$(LIBS_$(1)) -lgemma -lc -lgcc

endef

$(foreach TARGET,$(alltargets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(alltargets),$(foreach OBJ,$(COBJS) $(LIBIO_COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(alltargets))) > /dev/null 2>&1 || :)
endif
