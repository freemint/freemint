#
# Makefile for aranym.xfs (u:\host) in kernel compilation
#
SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = aranym

default: all

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_XFS__
COBJS += $(CMODOBJS)

# default definitions
all_compile_dirs = .compile_*
GENFILES = $(all_compile_dirs)

targets = 000 02060 deb

all-here: $(targets)

strip:
	$(STRIP) .compile_*/aranym.xfs

LDEXTRA = -nostdlib -Wl,--entry,init_xfs -Wl,--fatal-warnings

# phony alias so `make 02060` etc. builds the per-CPU artifact
000:: .compile_000/aranym.xfs
02060:: .compile_02060/aranym.xfs
deb:: .compile_deb/aranym.xfs

define TARGET_TEMPLATE

LIBKERN_NAME_$(1) = kern$$(CPU_$(1))
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS)
LIBS_$(1) = $$(LIBKERN_$(1)) -lgcc
OBJS_$(1) = $(addsuffix .o, $(addprefix .compile_$(1)/,$(notdir $(basename $(COBJS)))))

.compile_$(1)/aranym.xfs: $$(OBJS_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(1)) $$(LIBS_$(1))

endef

define LIBKERN_TEMPLATE
$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern libkern$(1).a
endef

$(foreach TARGET,$(targets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(targets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

# iterate unique CPUs reached by $(targets), not the full cputargets list
$(foreach TARGET,$(sort $(foreach T,$(targets),$(CPU_$(T)))),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(targets))) > /dev/null 2>&1 || :)
endif
