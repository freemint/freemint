#
# Makefile for I/O abstraction library
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = IO

default: all-here

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

include $(srcdir)/IODEFS

all-here: all-libs

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) libIO*.a

ALL_LIBS = $(foreach LIB,$(alltargets),libIO$(CPU_$(LIB)).a)

all-libs: $(ALL_LIBS)

#
# multi target stuff
#

define LIBIO_TARGET

$(1): libIO$$(CPU_$(1)).a)

OBJS_$(1) = $(foreach OBJ, $(notdir $(basename $(COBJS))), .compile_$(1)/$(OBJ).o)
DEFINITIONS_$(1) = $(DEFINITIONS)

libIO$$(CPU_$(1)).a: $$(OBJS_$(1))
	$(AR) rcs $$@.$$$$$$$$ $$(OBJS_$(1)) && mv $$@.$$$$$$$$ $$@ || { $(RM) $$@.$$$$$$$$; false; }
endef

$(foreach LIB,$(alltargets),$(eval $(call LIBIO_TARGET,$(LIB))))

$(foreach LIB,$(alltargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(LIB),$(OBJ)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(alltargets))) > /dev/null 2>&1 || :)
endif
