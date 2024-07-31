#
# Makefile for 
#
SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = audio

default: all

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

audiotargets = 000 02060 030 040 060

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_XDD__

# default definitions
compile_all_dirs = $(addprefix .compile_,$(audiotargets))
GENFILES = $(compile_all_dirs) actrl *.xdd

ALL_TARGETS = $(addsuffix /audiodev.xdd, $(addprefix .compile_,$(audiotargets))) $(addsuffix /actrl, $(addprefix .compile_,$(audiotargets)))

STRIPFLAGS =
strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

LDEXTRA = -nostdlib -Wl,--entry,init_xdd -Wl,--fatal-warnings

define TARGET_TEMPLATE

LIBKERN_NAME_$(1) = kern$$(CPU_$(1))
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS) $$(AUDIODEFS_$(1))
LIBS_$(1) = $$(LIBKERN_$(1)) -lgcc
OBJS_$(1) = $(addsuffix .o, $(addprefix .compile_$(1)/,$(notdir $(basename $(COBJS)))))

$(1): $(addprefix .compile_,$(1))/audiodev.xdd $(addprefix .compile_,$(1))/actrl

$(addprefix .compile_,$(1))/audiodev.xdd: $$(OBJS_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(1)) $$(LIBS_$(1))

$(addprefix .compile_,$(1))/actrl: $(srcdir)/actrl.c
	$(CC) $(LINKSTRIP) $$(MODEL_$$(CPU_$(1))) $(OPTS) $(WARN) $$< -o $$@

endef

define LIBKERN_TEMPLATE
$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern libkern$(1).a
endef

$(foreach TARGET,$(audiotargets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(audiotargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(audiotargets))) > /dev/null 2>&1 || :)
endif
