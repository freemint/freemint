#
# Makefile for libkern.a
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = libkern

default: help

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-libs

# default overwrites
DEFINITIONS = -D__KERNEL__

NOCFLAGS-strchr.c = -Wcast-qual
NOCFLAGS-strrchr.c = -Wcast-qual
NOCFLAGS-strstr.c = -Wcast-qual
NOCFLAGS-strtol.c = -Wcast-qual
NOCFLAGS-strtoll.c = -Wcast-qual
NOCFLAGS-strtoul.c = -Wcast-qual
NOCFLAGS-strtoull.c = -Wcast-qual

# default definitions
GENFILES = .compile_* libkern*.a


help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(cputargets)'
	@echo '#'
	@echo '# - bakclean'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - help'
	@echo '#'
	@echo '# example for a 030 libkern: -> make 030'
	@echo '#'

ALL_LIBS = $(foreach TARGET,$(cputargets),libkern$(TARGET).a)

all-libs: $(ALL_LIBS)

define LIBKERN_TARGET

$(1): libkern$(1).a

OBJS_$(1) = $(addsuffix .o, $(addprefix .compile_$(1)/,$(notdir $(basename $(COBJS)))))
DEFINITIONS_$(1) = $(DEFINITIONS)

libkern$(1).a: $$(OBJS_$(1))
	$(AR) rcs $$@.tmp $$(OBJS_$(1)) && mv $$@.tmp $$@ || { $(RM) $$@.tmp; false; }
endef

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TARGET,$(TARGET))))

$(foreach TARGET,$(cputargets),$(foreach OBJ,$(COBJS),$(eval $(call CPU_TEMPLATE,$(TARGET),$(OBJ)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(cputargets))) > /dev/null 2>&1 || :)
endif
