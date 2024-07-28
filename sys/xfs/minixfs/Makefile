#
# Makefile for minixfs
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = minixfs

default: help

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(srcdir)/MINIXDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites
DEFINITIONS = -D__KERNEL_XFS__
INCLUDES += -I$(top_srcdir)/../../sys

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) *.xfs

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(minixtargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

STRIPFLAGS = $(shell for i in `cat syms.mfs` ; do echo -n "-K $$i " ; done)
strip:
	$(STRIP) $(ALL_TARGETS)

ALL_TARGETS = $(addsuffix /minix.xfs, $(addprefix .compile_,$(minixtargets)))

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

LD = $(CC) -nostdlib -Wl,--entry,init_xfs -Wl,--fatal-warnings -s

define TARGET_TEMPLATE

# hack for libkern: CPU is 02060, but library is libkern020-60.a; fixme
LIBKERN_NAME_$(1) = kern$(subst 02060,020-60,$(CPU_$(1)))
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS) $$(MINIXDEFS_$(1))
LIBS_$(1) = $$(LIBKERN_$(1)) -lgcc
OBJS_$(1) = $(addprefix .compile_$(1)/,$(COBJS:.c=.o))

$(1): $(addprefix .compile_,$(1))/minix.xfs

$(addprefix .compile_,$(1))/minix.xfs: $$(OBJS_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(1)) $$(LIBS_$(1))

endef

define LIBKERN_TEMPLATE
$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern CPU="$(1)" libkern$(1).a
endef

$(foreach TARGET,$(minixtargets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(minixtargets),$(foreach OBJ,$(COBJS:.c=),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(minixtargets))) > /dev/null 2>&1 || :)
endif
