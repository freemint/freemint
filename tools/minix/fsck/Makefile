#
# Makefile for minixfs checker
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = fsck

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

include $(srcdir)/../MINIXDEFS

all-here: all-targets

# default overwrites
INCLUDES = -I$(top_srcdir)/IO

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

ALL_TARGETS = $(foreach TARGET,$(alltargets),.compile_$(TARGET)/fsck.minix)

strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

define TARGET_TEMPLATE

$(1): .compile_$(1)/fsck.minix

LIBIO_NAME_$(1) = IO$$(CPU_$(1))
LIBIO_DEP_$(1) = $(top_srcdir)/IO/lib$$(LIBIO_NAME_$(1)).a
LIBIO_$(1) = -L$(top_srcdir)/IO -l$$(LIBIO_NAME_$(1))
LIBS_$(1) = $$(LIBIO_$(1))
OBJS_$(1) = $(foreach OBJ, $(notdir $(basename $(COBJS))), .compile_$(1)/$(OBJ).o)
DEFINITIONS_$(1) = $(DEFINITIONS)

.compile_$(1)/fsck.minix: $$(OBJS_$(1)) $$(LIBIO_DEP_$(1))
	$(LD) $$(LDEXTRA_$(1)) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(1)) $$(LIBS_$(1))

endef

$(foreach TARGET,$(alltargets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(alltargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(alltargets))) > /dev/null 2>&1 || :)
endif
