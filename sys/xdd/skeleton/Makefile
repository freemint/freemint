#
# Makefile for a xdd skeleton
#
SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = skeleton

default: all

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

dummytargets = 000 02060 030 040 060 deb col

XDDDEFS_deb = -DDEV_DEBUG

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_XDD__

# default definitions
compile_all_dirs = $(addprefix .compile_,$(dummytargets))
GENFILES = $(compile_all_dirs) *.xdd

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(dummytargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'


ALL_TARGETS = $(addsuffix /dummy.xdd, $(addprefix .compile_,$(dummytargets)))

STRIPFLAGS =
strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

LD = $(CC) -nostdlib -Wl,--entry,init_xdd -Wl,--fatal-warnings

define TARGET_TEMPLATE

# hack for libkern: CPU is 02060, but library is libkern020-60.a; fixme
LIBKERN_NAME_$(1) = kern$(subst 02060,020-60,$(CPU_$(1)))
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS) $$(XDDDEFS_$(1))
LIBS_$(1) = $$(LIBKERN_$(1)) -lgcc
OBJS_$(1) = $(addprefix .compile_$(1)/,$(COBJS:.c=.o)) $(addprefix .compile_$(1)/,$(SOBJS:.S=.o))

$(1): $(addprefix .compile_,$(1))/dummy.xdd

$(addprefix .compile_,$(1))/dummy.xdd: $$(OBJS_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(1)) $$(LIBS_$(1))

endef

define LIBKERN_TEMPLATE
$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern CPU="$(1)" libkern$(1).a
endef

$(foreach TARGET,$(dummytargets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(dummytargets),$(foreach OBJ,$(COBJS:.c=),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(dummytargets),$(foreach OBJ,$(SOBJS:.S=),$(eval $(call AS_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(dummytargets))) > /dev/null 2>&1 || :)
endif
