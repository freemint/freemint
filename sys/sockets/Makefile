#
# Makefile for inet4
#

SHELL = /bin/sh
SUBDIRS = inet4 xif

srcdir = .
top_srcdir = ..
subdir = sockets

default: help

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(srcdir)/INET4DEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites
DEFINITIONS = -D__KERNEL_XDD__
INCLUDES += -I$(top_srcdir)/../../sys

# default definitions
compile_all_dirs = $(addprefix .compile_,$(inet4targets))
GENFILES = $(compile_all_dirs)

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(inet4targets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

ALL_TARGETS = $(addsuffix /inet4.xdd, $(addprefix .compile_,$(inet4targets)))

STRIPFLAGS =
strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

LDEXTRA = -nostdlib -Wl,--entry,init -Wl,--fatal-warnings

define TARGET_TEMPLATE

LIBKERN_NAME_$(1) = kern$$(CPU_$(1))
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS) $$(INET4DEFS_$(1))
LIBINET4_DEP_$(1) = inet4/.compile_$(1)/libinet4.a
LIBINET4_$(1) = -Linet4/.compile_$(1) -linet4
LIBS_$(1) = $$(LIBINET4_$(1)) $$(LIBKERN_$(1)) -lgcc
OBJS_$(1) = $(addsuffix .o, $(addprefix .compile_$(1)/,$(notdir $(basename $(COBJS)))))

$$(LIBINET4_DEP_$(1))::
	$(MAKE) -C inet4 $(1)

$(1): $(addprefix .compile_,$(1))/inet4.xdd

$(addprefix .compile_,$(1))/inet4.xdd: $$(OBJS_$(1)) $$(LIBINET4_DEP_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(1)) $$(LIBS_$(1))
	$(STRIP) $$@

endef

define LIBKERN_TEMPLATE
$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern libkern$(1).a
endef

$(foreach TARGET,$(inet4targets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(inet4targets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(inet4targets))) > /dev/null 2>&1 || :)
endif
