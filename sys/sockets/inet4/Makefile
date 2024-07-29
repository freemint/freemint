#
# Makefile for libinet4
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = 

default: help

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(srcdir)/../INET4DEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites
DEFINITIONS = -D__KERNEL_XDD__
INCLUDES = -I$(srcdir)/.. -I$(top_srcdir)

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
	@echo '# - help'
	@echo '#'

ALL_TARGETS = $(addsuffix /libinet4.a, $(addprefix .compile_,$(inet4targets)))

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

define TARGET_TEMPLATE

DEFINITIONS_$(1) = $(DEFINITIONS) $$(INET4DEFS_$(1))
OBJS_$(1) = $(addsuffix .o, $(addprefix .compile_$(1)/,$(notdir $(basename $(COBJS)))))

$(1): $(addprefix .compile_,$(1))/libinet4.a

$(addprefix .compile_,$(1))/libinet4.a: $$(OBJS_$(1))
	$(AR) rcs $$@.$$$$$$$$ $$(OBJS_$(1)) && mv $$@.$$$$$$$$ $$@ || { $(RM) $$@.$$$$$$$$; false; }

endef

$(foreach TARGET,$(inet4targets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(inet4targets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(inet4targets))) > /dev/null 2>&1 || :)
endif
