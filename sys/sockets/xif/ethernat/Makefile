#
# Makefile for ethernat
#
SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../../..
subdir = ethernat

default: all

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

targets = 060

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

DRIVERS = $(addsuffix /ethernat.xif, $(addprefix .compile_,$(targets)))

# default overwrites
INCLUDES = -I../.. -I$(top_srcdir)
XIF_DEFINITIONS =
DEFINITIONS = -D__KERNEL_XDD__ $(XIF_DEFINITIONS)

# default definitions
compile_all_dirs = $(addprefix .compile_,$(targets))
GENFILES = $(compile_all_dirs) *.xif

all-here: $(DRIVERS)

strip:
	$(STRIP) $(DRIVERS)

#
# multi target stuff
#

LDEXTRA = -nostdlib -Wl,--entry,init -Wl,--fatal-warnings

define TARGET_TEMPLATE

LIBKERN_NAME_$(1) = kern$(1)
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS) $$(XIF_DEFS_$(1))
LIBS_$(1) = $$(LIBKERN_$(1)) -lgcc
OBJS_$(1) = $(addsuffix .o, $(addprefix .compile_$(1)/,$(notdir $(basename $(COBJS)))))

$(addprefix .compile_,$(1))/ethernat.xif: $$(OBJS_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) -o $$@ $$(CFLAGS_$(1)) $$(OBJS_$(1)) $$(LIBS_$(1))

endef

define LIBKERN_TEMPLATE
$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern libkern$(1).a
endef

# driver target

$(foreach TARGET,$(targets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(targets),$(foreach OBJ,$(COBJS),$(eval $(call CPU_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(targets))) > /dev/null 2>&1 || :)
endif
