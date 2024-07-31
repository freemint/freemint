#
# the m68k arch specific files
#

SHELL = /bin/sh
SUBDIRS = magic

srcdir = .
top_srcdir = ..
subdir = arch

default: all

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = $(KERNELDEFAULTDEFS) $(KERNELDEFS)

# default definitions
compile_all_dirs = .compile_* 
GENFILES = $(compile_all_dirs)

ALL_OBJS = $(foreach TARGET, $(kerneltargets), .compile_$(TARGET)/libarch.a .compile_$(TARGET)/startup.o)

all-here: $(ALL_OBJS)

#
# main target
#
define ARCH_TARGET

DEFINITIONS_$(1) = $(DEFINITIONS) $$(KERNELDEFS_$(1))
OBJS_$(1) = $(addsuffix .o, $(addprefix .compile_$(1)/,$(notdir $(basename $(COBJS)))))

$(1): .compile_$(1)/libarch.a  .compile_$(1)/startup.o

.compile_$(1)/libarch.a: $$(OBJS_$(1))
	$(AR) rcs $$@.tmp $$(OBJS_$(1)) && mv $$@.tmp $$@ || { $(RM) $$@.tmp; false; }

$$(OBJS_$(1)): $(srcdir)/magic/magic.i
endef

$(foreach TARGET,$(kerneltargets),$(eval $(call ARCH_TARGET,$(TARGET))))

$(foreach TARGET,$(kerneltargets),$(foreach OBJ,$(COBJS) $(STARTUP),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))


$(srcdir)/magic/magic.i:
	$(MAKE) -C $(srcdir)/magic magic.i


ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(kerneltargets))) > /dev/null 2>&1 || :)
endif
